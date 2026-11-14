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
OutputBaseFilename=NINA-Setup-{#MyAppVersion}
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
; Il VST3 va nella cartella comune dei plugin (serve l'amministratore); chi non e' amministratore puo' installare per il solo utente corrente
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=commandline dialog
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

[Types]
Name: "full"; Description: "{cm:TypeFull}"
Name: "custom"; Description: "{cm:TypeCustom}"; Flags: iscustom

[Components]
Name: "standalone"; Description: "{cm:CompStandalone}"; Types: full custom
Name: "vst3"; Description: "{cm:CompVst3}"; Types: full custom
Name: "docs"; Description: "{cm:CompDocs}"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "{cm:TaskDesktop}"; Components: standalone; Flags: unchecked

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
// L'attributo di sistema sulla cartella .vst3 fa mostrare in Esplora risorse l'icona del plugin (desktop.ini)
procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if (CurStep = ssPostInstall) and WizardIsComponentSelected('vst3') then
    Exec(ExpandConstant('{cmd}'), ExpandConstant('/C attrib +s "{autocf}\VST3\NINA.vst3"'), '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;
