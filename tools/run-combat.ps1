param(
  [string]$PCSX2Path = 'C:/Users/leogn/OneDrive/Documents/pcsx2-v2.3.362-windows-x64-Qt/pcsx2-qt.exe',
  [switch]$ShowWindow
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$run = Join-Path $repo 'game/bin'
$elf = Join-Path $run 'droid_campos.elf'
$template = Join-Path $repo 'artifacts/optimization/pcsx2-validation/PCSX2/inis/PCSX2.ini'
foreach ($required in @($PCSX2Path, $elf, (Join-Path $run 'enemies/leader/motion.bin'),
                        $template)) {
  if (-not (Test-Path -LiteralPath $required)) {throw "Arquivo ausente: $required"}
}
if (Get-Process -Name 'pcsx2-qt' -ErrorAction SilentlyContinue) {
  throw 'Ja existe uma instancia do PCSX2. Feche-a antes de iniciar o teste isolado.'
}
$artifacts = Join-Path $repo 'artifacts/combat'
New-Item -ItemType Directory -Path $artifacts -Force | Out-Null
$profile = Join-Path $artifacts 'pcsx2-profile'
$iniDirectory = Join-Path $profile 'PCSX2/inis'
New-Item -ItemType Directory -Path $iniDirectory -Force | Out-Null
$targetIni = Join-Path $iniDirectory 'PCSX2.ini'
if (-not (Test-Path -LiteralPath $targetIni)) {
  Copy-Item -LiteralPath $template -Destination $targetIni
}
# Validate against retail PS2 RAM, not the emulator's 128 MiB development mode.
$iniText = Get-Content -LiteralPath $targetIni -Raw
$iniText = $iniText -replace '(?m)^ExtraMemory\s*=.*$', 'ExtraMemory = false'
Set-Content -LiteralPath $targetIni -Value $iniText -NoNewline -Encoding utf8
$log = Join-Path $artifacts ('pcsx2-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.log')
$arguments = '-batch -nofullscreen -datapath "' + $profile + '" -logfile "' + $log + '" -elf "' + $elf + '"'
$style = 'Hidden'
if ($ShowWindow) {$style = 'Normal'} else {$arguments = '-nogui ' + $arguments}
$instance = Start-Process -FilePath $PCSX2Path -ArgumentList $arguments -WorkingDirectory $run -WindowStyle $style -PassThru
Write-Output "PCSX2 PID: $($instance.Id)"
Write-Output "Log: $log"
