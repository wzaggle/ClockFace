#define Version "1.1.0"
[Setup]
AppId=OBS-Clock
AppName=OBS Clock
AppVersion={#Version}
DefaultDirName={commonappdata}\obs-studio\plugins\OBS-Clock
DisableProgramGroupPage=yes
PrivilegesRequired=admin
OutputDir=..\packages\Windows
OutputBaseFilename=OBS-Clock-Setup-{#Version}-Windows-x64
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName=OBS Clock
CloseApplications=no
[Files]
Source: "..\stage\OBS-Clock\OBS-Clock.dll"; DestDir: "{app}"; Flags: ignoreversion
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
   if Result then WizardForm.DirEdit.Text := AddBackslash(Root) + 'plugins\OBS-Clock';
  end else WizardForm.DirEdit.Text := ExpandConstant('{commonappdata}\obs-studio\plugins\OBS-Clock');
 end;
end;
