param(
  [string]$PCSX2Path = 'C:/Users/leogn/OneDrive/Documents/pcsx2-v2.3.362-windows-x64-Qt/pcsx2-qt.exe',
  [switch]$ShowWindow
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$artifacts = Join-Path $repo 'artifacts/optimization'
$run = Join-Path $artifacts 'run-integrated'
$profile = Join-Path $artifacts 'pcsx2-validation'
$sourceElf = Join-Path $artifacts 'hud-step5.elf'
$targetElf = Join-Path $run 'droid_campos.elf'
foreach ($required in @($PCSX2Path, $sourceElf, (Join-Path $run 'earthbound-Font.png'), (Join-Path $profile 'PCSX2/inis/PCSX2.ini'))) {
  if (-not (Test-Path -LiteralPath $required)) {throw "Arquivo ausente: $required"}
}
if (Get-Process -Name 'pcsx2-qt' -ErrorAction SilentlyContinue) {
  throw 'Ja existe uma instancia do PCSX2. Feche-a antes de iniciar este teste isolado.'
}
Copy-Item -LiteralPath $sourceElf -Destination $targetElf
$log = Join-Path $artifacts ('pcsx2-test-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.log')
$arguments = '-batch -nofullscreen -datapath "' + $profile + '" -logfile "' + $log + '" -elf "' + $targetElf + '"'
$style = 'Hidden'
if ($ShowWindow) {$style = 'Normal'} else {$arguments = '-nogui ' + $arguments}
$instance = Start-Process -FilePath $PCSX2Path -ArgumentList $arguments -WorkingDirectory $run -WindowStyle $style -PassThru
Write-Output "PCSX2 PID: $($instance.Id)"
Write-Output "Log: $log"
