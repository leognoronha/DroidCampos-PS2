$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildLog = Join-Path $projectRoot 'game/build-etip.log'
$containerBuild = 'set -eu; rsync -ac --exclude .git --exclude obj --exclude bin --exclude build-etip.log /host/ /src/; rsync -ac --delete /host/game/src/ /src/game/src/; rsync -ac --delete /host/game/inc/ /src/game/inc/; cd /src/game; make build-engine all; test -s /src/game/bin/droid_campos.elf; test -s /src/game/bin/etip_inside/porta.obj; rsync -ac /src/game/bin/ /host/game/bin/'
Push-Location $projectRoot
try {
    & docker exec tyra-compiler sh -lc $containerBuild 2>&1 | Tee-Object -FilePath $buildLog
    $buildExit = $LASTEXITCODE
    if ($buildExit -ne 0) {
        throw "Build falhou (codigo $buildExit). Log: $buildLog"
    }
    $binary = Get-Item -LiteralPath (Join-Path $projectRoot 'game/bin/droid_campos.elf')
    Write-Host "Build concluido: $($binary.FullName) ($($binary.Length) bytes)."
    Write-Host "Assets incluidos em game/bin/etip_inside. Log: $buildLog"
} finally {
    Pop-Location
}
