"""Build the pinned ClockFace revision against official OBS 33 host libraries."""
import argparse, hashlib, json, os, platform, plistlib, shutil, subprocess, tarfile, urllib.request, zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SPEC=json.loads((ROOT/'ci/dependencies.json').read_text())
VERSION=(ROOT/'VERSION').read_text().strip()
SDK=ROOT/'.ci-sdk'

def run(*args,**kwargs):
 print('+', ' '.join(map(str,args)),flush=True)
 return subprocess.run(list(map(str,args)),cwd=ROOT,check=True,**kwargs)

def fetch(url,sha):
 cache=ROOT/'.ci-downloads';cache.mkdir(exist_ok=True);dest=cache/url.rsplit('/',1)[-1]
 if not dest.exists() or hashlib.sha256(dest.read_bytes()).hexdigest()!=sha:
  with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'ClockFace-build'}),timeout=120) as response,dest.open('wb') as out:shutil.copyfileobj(response,out)
 if hashlib.sha256(dest.read_bytes()).hexdigest()!=sha:raise RuntimeError('Checksum mismatch: '+dest.name)
 return dest

def asset(key):
 item=SPEC['assets'][key]
 return fetch('https://github.com/obsproject/obs-studio/releases/download/'+SPEC['obs_version']+'/'+item['name'],item['sha256'])

def unpack(archive,dest):
 dest.mkdir(parents=True,exist_ok=True)
 with tarfile.open(archive) as source:source.extractall(dest,filter='data')

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--platform',choices=['windows','macos-arm64','macos-x86_64','linux'],required=True);kind=parser.parse_args().platform
 expected='Darwin' if kind.startswith('macos') else 'Windows' if kind=='windows' else 'Linux'
 if platform.system()!=expected:raise RuntimeError('Use native '+expected+' runner')
 SDK.mkdir(exist_ok=True);unpack(asset('sources'),SDK/'source')
 headers=next(p.parent for p in (SDK/'source').rglob('obs-module.h'))
 generated=SDK/'generated';generated.mkdir(exist_ok=True)
 (generated/'obsconfig.h').write_text('#pragma once\n#define OBS_RELEASE_CANDIDATE 0\n#define OBS_BETA 0\n')
 options=[];env=os.environ.copy()
 if kind=='windows':
  vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
  install=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
  script=Path(install)/'Common7/Tools/VsDevCmd.bat'
  result=subprocess.check_output(f'"{script}" -arch=x64 >nul && set',shell=True,text=True)
  for line in result.splitlines():
   if '=' in line and not line.startswith('='):
    key,value=line.split('=',1);env[key]=value
  # Windows resolves executable paths using this process environment.
  os.environ.update(env);env=os.environ.copy()
  host=SDK/'obs-windows';host.mkdir(exist_ok=True)
  with zipfile.ZipFile(asset(kind)) as z:z.extractall(host)
  library=SDK/'obs.lib'
  run('lib.exe','/nologo','/machine:x64','/def:'+str(ROOT/'ci/obs-imports.def'),'/out:'+str(library),env=env)
 elif kind.startswith('macos'):
  mount=SDK/'mount';mount.mkdir(exist_ok=True)
  run('hdiutil','attach',asset(kind),'-mountpoint',mount,'-nobrowse')
  try:
   app=next(mount.glob('*.app'));shutil.copytree(app,SDK/'OBS.app',symlinks=True,dirs_exist_ok=True)
  finally:run('hdiutil','detach',mount)
  libraries=[p for p in (SDK/'OBS.app/Contents/Frameworks').rglob('*') if p.is_file() and (p.name=='libobs' or p.name=='libobs.0.dylib')]
  if not libraries:raise RuntimeError('OBS host library not found')
  library=libraries[0]
  deps=SPEC['macos_prebuilt'];unpack(fetch(deps['url'],deps['sha256']),SDK/'prebuilt')
  simde=next((SDK/'prebuilt').rglob('simde/x86/sse2.h'))
  options+=['-DOBS_EXTRA_INCLUDE_DIR='+str(simde.parents[2])]
  qt=SPEC['macos_qt'];unpack(fetch(qt['url'],qt['sha256']),SDK/'qt')
  qtconfig=next((SDK/'qt').rglob('Qt6Config.cmake'))
  # Qt6Config.cmake lives at <prefix>/lib/cmake/Qt6.
  prefix=qtconfig.parents[3]
  options+=['-DCMAKE_PREFIX_PATH='+str(prefix),'-DCMAKE_OSX_ARCHITECTURES='+kind.split('-',1)[1],'-DCMAKE_OSX_DEPLOYMENT_TARGET=13.0']
  env['DYLD_FRAMEWORK_PATH']=str(SDK/'OBS.app/Contents/Frameworks')+':'+str(prefix/'lib')
 else:
  run('dpkg-deb','-x',asset(kind),SDK/'obs-linux')
  libraries=list((SDK/'obs-linux').rglob('libobs.so.*'))
  if not libraries:raise RuntimeError('OBS host library not found')
  library=next(p for p in libraries if p.is_file())
  options+=['-DCMAKE_INSTALL_LIBDIR=lib/x86_64-linux-gnu']
 build=ROOT/'out'/kind
 args=['cmake','-S',ROOT,'-B',build,'-DCMAKE_BUILD_TYPE=Release','-DOBS_INCLUDE_DIR='+str(headers),'-DOBS_GENERATED_INCLUDE_DIR='+str(generated),'-DOBS_IMPORT_LIBRARY='+str(library),*options]
 # VsDevCmd already selects x64. Ninja works with that compiler environment
 # and avoids CMake choosing NMake, which does not accept -A.
 args+=['-G','Ninja']
 run(*args,env=env);run('cmake','--build',build,'--config','Release','--parallel','4',env=env)
 env['QT_QPA_PLATFORM']='offscreen'
 run('ctest','--test-dir',build,'-C','Release','--output-on-failure',env=env)
 stage=ROOT/'stage';prefix=stage/'usr' if kind=='linux' else stage
 run('cmake','--install',build,'--config','Release','--prefix',prefix,env=env)
 out=ROOT/'packages'/kind;out.mkdir(parents=True,exist_ok=True)
 if kind=='windows':
  # Inno output is moved into the same platform directory as metadata.
  candidates=[Path(os.environ.get('ProgramFiles(x86)',''))/'Inno Setup 6/ISCC.exe',Path(os.environ.get('ProgramFiles',''))/'Inno Setup 6/ISCC.exe']
  iscc=next((p for p in candidates if p.exists()),None)
  if not iscc:raise RuntimeError('Inno Setup 6 is required')
  run(iscc,ROOT/'installer/windows.iss')
  setup=next((ROOT/'packages/Windows').glob('*.exe'));shutil.move(setup,out/setup.name)
  shutil.make_archive(str(out/f'ClockFace-{VERSION}-Windows-x64-manual'),'zip',stage)
 elif kind.startswith('macos'):
  bundle=stage/'ClockFace.plugin';binary=bundle/'Contents/MacOS/ClockFace'
  run('lipo',binary,'-verify_arch',kind.split('-',1)[1]);run('otool','-L',binary)
  run('codesign','--force','--deep','--sign','-',bundle);run('codesign','--verify','--deep','--strict',bundle)
  pkgroot=ROOT/'package-root';target=pkgroot/'Library/Application Support/obs-studio/plugins/ClockFace.plugin'
  shutil.copytree(bundle,target,symlinks=True,dirs_exist_ok=True)
  components=ROOT/'components.plist';run('pkgbuild','--analyze','--root',pkgroot,components)
  records=plistlib.loads(components.read_bytes())
  for record in records:record['BundleIsRelocatable']=False
  components.write_bytes(plistlib.dumps(records))
  component=ROOT/'ClockFace-component.pkg'
  run('pkgbuild','--identifier','com.wzaggle.clockface.user','--version',VERSION,'--root',pkgroot,'--install-location','/','--component-plist',components,component)
  distribution=ROOT/'distribution.xml'
  distribution.write_text(f'''<installer-gui-script minSpecVersion="2"><title>ClockFace {VERSION}</title>
<options customize="never" require-scripts="false"/>
<domains enable_anywhere="false" enable_currentUserHome="true" enable_localSystem="false"/>
<choices-outline><line choice="clock"/></choices-outline>
<choice id="clock" visible="false"><pkg-ref id="com.wzaggle.clockface.user"/></choice>
<pkg-ref id="com.wzaggle.clockface.user" version="{VERSION}" auth="none">ClockFace-component.pkg</pkg-ref></installer-gui-script>''')
  setup=out/f'ClockFace-Setup-{VERSION}-{kind}.pkg';run('productbuild','--distribution',distribution,'--package-path',ROOT,setup)
  run('installer','-pkg',setup,'-target','CurrentUserHomeDirectory')
  installed=Path.home()/'Library/Application Support/obs-studio/plugins/ClockFace.plugin/Contents/MacOS/ClockFace'
  if not installed.exists() or installed.read_bytes()!=binary.read_bytes():raise RuntimeError('Installer verification failed')
  with tarfile.open(out/f'ClockFace-{VERSION}-{kind}-manual.tar.gz','w:gz') as archive:archive.add(bundle,arcname=bundle.name)
 else:
  expected_plugin=stage/'usr/lib/x86_64-linux-gnu/obs-modules/plugins/ClockFace.so'
  if not expected_plugin.is_file():raise RuntimeError('OBS 33 Linux plugin layout missing')
  control=stage/'DEBIAN';control.mkdir(exist_ok=True)
  # Derived package requirements include the Qt version used by this runner.
  qtversion=subprocess.check_output(['pkg-config','--modversion','Qt6Gui'],text=True).strip()
  def package_owner(soname):
   libdir=subprocess.check_output(['pkg-config','--variable=libdir','Qt6Gui'],text=True).strip()
   output=subprocess.check_output(['dpkg-query','-S',str(Path(libdir)/soname)],text=True).strip()
   return output.split(':',1)[0]
  gui_package=package_owner('libQt6Gui.so.6');core_package=package_owner('libQt6Core.so.6')
  (control/'control').write_text(f'Package: clockface\nConflicts: obs-clock\nReplaces: obs-clock\nVersion: {VERSION}\nArchitecture: amd64\nMaintainer: William Zaggle\nSection: video\nPriority: optional\nDepends: obs-studio (>= 33.0.1), {gui_package} (>= {qtversion}), {core_package} (>= {qtversion}), libstdc++6\nDescription: ClockFace analog clock and scheduled message source\n')
  run('dpkg-deb','--root-owner-group','--build',stage,out/f'ClockFace-{VERSION}-Ubuntu26.04-amd64.deb')
  run('dpkg-deb','--info',next(out.glob('*.deb')))
 for document in [ROOT/'USER.md',ROOT/'LICENSE',ROOT/f'docs/ClockFace-User-Guide-v{VERSION}.docx']:
  shutil.copy2(document,out/document.name)
 revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
 (out/'BUILD-INFO.json').write_text(json.dumps({'product':'ClockFace','version':VERSION,'revision':revision,'platform':kind,'obs_sdk':SPEC['obs_version'],'automated_tests':'passed','obs_runtime_test':'not performed by CI','signing':'ad-hoc, not notarized' if kind.startswith('macos') else 'unsigned test installer'},indent=2)+'\n')
 (out/'INSTALL.txt').write_text('Close OBS before installing. Use the installer for this platform. See the included ClockFace user guide. Native Linux DEB targets Ubuntu 26.04 and is not a Flatpak extension. macOS test packages are ad-hoc signed, not notarized.\n')
 (out/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in sorted(out.iterdir()) if p.is_file() and p.name!='SHA256SUMS.txt'))
if __name__=='__main__':main()
