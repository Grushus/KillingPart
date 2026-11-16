# Killing Part: runs the game from the editor build (no packaging) with test switches, waits, closes it.
# Usage: powershell -File Tools\run_game_test.ps1 -Seconds 40 -GameArgs "-KPGod -KPAutoCut -KPNight=1 -KPShots=4"
param([int]$Seconds = 40, [string]$GameArgs = "")
$proj = Join-Path $PSScriptRoot "..\KillingPart.uproject" | Resolve-Path
$ueDir = (Get-ItemProperty "HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.9" -ErrorAction SilentlyContinue).InstalledDirectory
if (-not $ueDir) { $ueDir = "C:\Program Files\Epic Games\UE_5.9" }
$exe = Join-Path $ueDir "Engine\Binaries\Win64\UnrealEditor.exe"
$all = "`"$proj`" -game -windowed -ResX=1280 -ResY=720 -nosound -log=KPTest.log $GameArgs"
$p = Start-Process -FilePath $exe -ArgumentList $all -PassThru
Start-Sleep -Seconds $Seconds
$p.CloseMainWindow() | Out-Null
Start-Sleep -Seconds 4
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
"closed"
