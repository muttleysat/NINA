; Installer di NINA per Windows (Inno Setup 6). Si compila con installer\build-installer.ps1, che passa la versione letta da CMakeLists.txt.
; Contiene tutto il necessario: lo Standalone (con il supporto ASIO gia' dentro l'eseguibile), il plugin VST3, il manuale e le licenze.
; Il runtime di Visual C++ e' collegato staticamente: non serve scaricare nient'altro.

#define MyAppName "NINA"
#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif
#define MyAppPublisher "Vinz Trombetta"
#define MyAppURL "https://github.com/muttleysat/NINA"
#define BuildDir "..\build\Nina_artefacts\Release"

[Setup]
; Identificativo fisso: installare una versione nuova sopra la vecchia la sostituisce (aggiornamento)
AppId={{0A7D79B3-A027-455A-84BE-E86F3C6BA83E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases
AppCopyright=Copyright (C) 2026 {#MyAppPublisher}
VersionInfoVersion={#MyAppVersion}
VersionInfoDescription=NINA - Drumbox Sequencer Powerhouse (installer)
DefaultDirName={autopf}\NINA
DefaultGroupName=NINA
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=NINA-Setup-{#MyAppVersion}-win-x64
SetupIconFile=..\Source\assets\nina.ico
UninstallDisplayIcon={app}\NINA.exe
UninstallDisplayName=NINA - Drumbox Sequencer Powerhouse
WizardStyle=modern
WizardImageFile=wizard-large-100.bmp,wizard-large-125.bmp,wizard-large-150.bmp,wizard-large-200.bmp
WizardSmallImageFile=wizard-small-100.bmp,wizard-small-125.bmp,wizard-small-150.bmp,wizard-small-200.bmp
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Parte senza amministratore; la scelta "per tutti gli utenti / solo per me" e' una pagina del wizard (vedi [Code]):
; per tutti gli utenti l'installer si rilancia con /ALLUSERS (UAC) e il VST3 va in Common Files\VST3 di sistema
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=commandline
CloseApplications=yes
RestartApplications=no
ChangesAssociations=no
SetupLogging=yes

[Languages]
; La lingua dell'installer segue quella di Windows (inglese se non e' disponibile); l'utente puo' sceglierla comunque
Name: "english"; MessagesFile: "compiler:Default.isl"; InfoBeforeFile: "info_en.txt"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"; InfoBeforeFile: "info_it.txt"

[CustomMessages]
english.CompStandalone=NINA Standalone (application with its own audio settings: ASIO, WASAPI, DirectSound)
english.CompVst3=NINA VST3 plug-in (for Cubase and other DAWs)
english.CompDocs=User manual and licences
english.TypeFull=Full installation (Standalone, VST3 and manual)
english.TypeCustom=Custom installation
english.TaskDesktop=Create a &desktop shortcut
english.LaunchNina=Launch NINA
english.OpenManual=Open the manual folder
english.ManualShortcut=NINA manual
english.UninstallShortcut=Uninstall NINA
italian.CompStandalone=NINA Standalone (applicazione con le sue impostazioni audio: ASIO, WASAPI, DirectSound)
italian.CompVst3=Plugin NINA VST3 (per Cubase e altre DAW)
italian.CompDocs=Manuale d'uso e licenze
italian.TypeFull=Installazione completa (Standalone, VST3 e manuale)
italian.TypeCustom=Installazione personalizzata
italian.TaskDesktop=Crea un collegamento sul &desktop
italian.LaunchNina=Avvia NINA
italian.OpenManual=Apri la cartella del manuale
italian.ManualShortcut=Manuale di NINA
italian.UninstallShortcut=Disinstalla NINA

english.ScopeTitle=Installation scope
english.ScopeDesc=Who should NINA be installed for?
english.ScopeAll=For all users of this computer (requires administrator permission)
english.ScopeMe=Only for me (no administrator permission needed)
english.ScopeElevateFailed=Administrator permission was not granted. Choose "Only for me" or try again.
italian.ScopeTitle=Ambito di installazione
italian.ScopeDesc=Per chi vuoi installare NINA?
italian.ScopeAll=Per tutti gli utenti del computer (richiede i permessi di amministratore)
italian.ScopeMe=Solo per me (non servono i permessi di amministratore)
italian.ScopeElevateFailed=I permessi di amministratore non sono stati concessi. Scegli "Solo per me" oppure riprova.

[Types]
Name: "full"; Description: "{cm:TypeFull}"
Name: "custom"; Description: "{cm:TypeCustom}"; Flags: iscustom

[Components]
Name: "standalone"; Description: "{cm:CompStandalone}"; Types: full custom
Name: "vst3"; Description: "{cm:CompVst3}"; Types: full custom
Name: "docs"; Description: "{cm:CompDocs}"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "{cm:TaskDesktop}"; Components: standalone; Flags: unchecked

[InstallDelete]
; Una copia "a file singolo" di NINA.vst3 (es. lasciata da una compilazione) impedirebbe di creare la cartella del plugin
Type: files; Name: "{autocf}\VST3\NINA.vst3"; Components: vst3

[Files]
Source: "{#BuildDir}\Standalone\NINA.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "{#BuildDir}\VST3\NINA.vst3\*"; DestDir: "{autocf}\VST3\NINA.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\docs\manuale\*"; DestDir: "{app}\Docs\manuale"; Components: docs; Flags: ignoreversion
Source: "..\docs\img\*"; DestDir: "{app}\Docs\img"; Components: docs; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Components: docs; Flags: ignoreversion

[Icons]
Name: "{group}\NINA"; Filename: "{app}\NINA.exe"; Components: standalone
Name: "{group}\{cm:ManualShortcut}"; Filename: "{app}\Docs\manuale\README.md"; Components: docs
Name: "{group}\{cm:UninstallShortcut}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\NINA"; Filename: "{app}\NINA.exe"; Tasks: desktopicon; Components: standalone

[Run]
Filename: "{app}\NINA.exe"; Description: "{cm:LaunchNina}"; Flags: nowait postinstall skipifsilent; Components: standalone
Filename: "{app}\Docs\manuale"; Description: "{cm:OpenManual}"; Flags: postinstall shellexec skipifsilent unchecked; Components: docs

[Code]
var
  ScopePage: TInputOptionWizardPage;
  CancelWithoutPrompt: Boolean;

procedure InitializeWizard;
begin
  // Se l'installer e' gia' elevato (rilanciato o /ALLUSERS) la pagina viene saltata
  ScopePage := CreateInputOptionPage(wpWelcome, CustomMessage('ScopeTitle'), CustomMessage('ScopeDesc'), '', True, False);
  ScopePage.Add(CustomMessage('ScopeAll'));
  ScopePage.Add(CustomMessage('ScopeMe'));
  ScopePage.SelectedValueIndex := 0;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := (PageID = ScopePage.ID) and IsAdminInstallMode;
end;

procedure CancelButtonClick(CurPageID: Integer; var Cancel, Confirm: Boolean);
begin
  if CancelWithoutPrompt then
    Confirm := False;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  ErrorCode: Integer;
  Started: Boolean;
begin
  Result := True;
  if (CurPageID = ScopePage.ID) and (ScopePage.SelectedValueIndex = 0) and not IsAdminInstallMode and not WizardSilent then
  begin
    // Rilancia lo stesso installer con i permessi di amministratore, nella stessa lingua, e chiude questo
    Started := ShellExec('runas', ExpandConstant('{srcexe}'), '/ALLUSERS /LANG=' + ActiveLanguage, '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
    if not Started then
    begin
      // Secondo tentativo tramite PowerShell (Start-Process -Verb RunAs)
      Log('ShellExec runas failed, error ' + IntToStr(ErrorCode) + '; trying PowerShell');
      Started := Exec(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'),
        '-NoProfile -WindowStyle Hidden -Command "Start-Process -FilePath ''' + ExpandConstant('{srcexe}') + ''' -ArgumentList ''/ALLUSERS'',''/LANG=' + ActiveLanguage + ''' -Verb RunAs"',
        '', SW_HIDE, ewWaitUntilTerminated, ErrorCode) and (ErrorCode = 0);
    end;
    if Started then
    begin
      CancelWithoutPrompt := True;
      WizardForm.Close;
    end
    else
    begin
      Log('Elevation failed, error ' + IntToStr(ErrorCode) + ' ' + SysErrorMessage(ErrorCode));
      MsgBox(CustomMessage('ScopeElevateFailed') + #13#10#13#10 + '(' + IntToStr(ErrorCode) + ': ' + SysErrorMessage(ErrorCode) + ')', mbError, MB_OK);
    end;
    Result := False;
  end;
end;

// L'attributo di sistema sulla cartella .vst3 fa mostrare in Esplora risorse l'icona del plugin (desktop.ini)
procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  // Prima di copiare i file l'attributo va tolto, altrimenti una reinstallazione/aggiornamento da' errore 183
  if (CurStep = ssInstall) and WizardIsComponentSelected('vst3') then
    Exec(ExpandConstant('{cmd}'), ExpandConstant('/C attrib -s "{autocf}\VST3\NINA.vst3"'), '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  if (CurStep = ssPostInstall) and WizardIsComponentSelected('vst3') then
    Exec(ExpandConstant('{cmd}'), ExpandConstant('/C attrib +s "{autocf}\VST3\NINA.vst3"'), '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;
