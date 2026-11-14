# Costruisce l'installer di NINA (installer\Output\NINA-Setup-<versione>.exe).
# Prima compila il progetto in Release (cmake --build build --config Release); serve Inno Setup 6 (winget install JRSoftware.InnoSetup).
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

$cmake = Get-Content (Join-Path $root 'CMakeLists.txt') -Raw
if ($cmake -notmatch 'project\(NINA VERSION ([0-9.]+)') { throw 'Versione non trovata in CMakeLists.txt' }
$version = $Matches[1]

foreach ($f in 'build\Nina_artefacts\Release\Standalone\NINA.exe', 'build\Nina_artefacts\Release\VST3\NINA.vst3\Contents\x86_64-win\NINA.vst3') {
    if (-not (Test-Path (Join-Path $root $f))) { throw "File mancante: $f (compila prima il progetto in Release)" }
}

$candidates = @("$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe", "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe")
$iscc = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { throw 'Inno Setup 6 non trovato (winget install JRSoftware.InnoSetup)' }

& $iscc "/DMyAppVersion=$version" (Join-Path $PSScriptRoot 'NINA.iss')
if ($LASTEXITCODE -ne 0) { throw "ISCC ha restituito $LASTEXITCODE" }
Write-Host "Installer creato: installer\Output\NINA-Setup-$version.exe"
