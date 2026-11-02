#define Version "1.1.1"
[Setup]
AppId=ClockFace
AppName=ClockFace
AppVersion={#Version}
DefaultDirName={commonappdata}\obs-studio\plugins\ClockFace
DisableProgramGroupPage=yes
PrivilegesRequired=admin
OutputDir=..\packages\Windows
OutputBaseFilename=ClockFace-Setup-{#Version}-Windows-x64
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName=ClockFace
CloseApplications=no
[Files]
Source: "..\stage\ClockFace\ClockFace.dll"; DestDir: "{app}"; Flags: ignoreversion
[InstallDelete]
; Remove only the specifically named earlier test DLL next to ClockFace.
Type: files; Name: "{app}\..\OBS-Clock\OBS-Clock.dll"
[Code]
var Mode: TInputOptionWizardPage;
procedure InitializeWizard;
begin
 Mode := CreateInputOptionPage(wpWelcome, 'OBS installation type', 'Close OBS before continuing.', 'Choose where this plugin will be installed.', True, False);
 Mode.Add('Standard OBS installation');
 Mode.Add('Portable OBS installation');
 Mode.SelectedValueIndex := 0;
end;
function NextButtonClick(CurPageID: Integer): Boolean;
var Root: String;
begin
 Result := True;
 if CurPageID = Mode.ID then begin
  if Mode.SelectedValueIndex = 1 then begin
   Root := '';
   Result := BrowseForFolder('Select the portable OBS root folder containing bin', Root, False);
   if Result then WizardForm.DirEdit.Text := AddBackslash(Root) + 'plugins\ClockFace';
  end else WizardForm.DirEdit.Text := ExpandConstant('{commonappdata}\obs-studio\plugins\ClockFace');
 end;
end;
