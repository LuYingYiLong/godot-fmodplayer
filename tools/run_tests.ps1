param(
    [ValidateSet('profile', 'spectrum', 'all')]
    [string]$Suite = 'all',
    [ValidatePattern('^[a-zA-Z0-9_-]+$')]
    [string]$Label = 'run',
    [string]$GodotPath = 'C:/Users/Administrator/Documents/Godot/Godot_v4.7.2-stable_win64.exe',
    [string]$FmodRuntime,
    [switch]$PrepareOnly,
    [switch]$Import,
    [switch]$Quick
)

$ErrorActionPreference = 'Stop'
function Sync-TestFile([string]$SourcePath, [string]$TargetPath) {
    if ((Test-Path -LiteralPath $TargetPath) -and
        ((Get-FileHash -LiteralPath $SourcePath).Hash -eq (Get-FileHash -LiteralPath $TargetPath).Hash)) {
        return
    }
    Copy-Item -LiteralPath $SourcePath -Destination $TargetPath -Force
}
$repoRoot = Split-Path -Parent $PSScriptRoot
$testRoot = Join-Path $repoRoot 'tests'
$binRoot = Join-Path $testRoot 'bin'
$runName = $Label + '_' + (Get-Date -Format 'yyyyMMdd_HHmmss_fff')
$logRoot = Join-Path $testRoot ('logs/' + $runName)
New-Item -ItemType Directory -Path $binRoot, $logRoot -Force | Out-Null
[System.IO.File]::WriteAllText((Join-Path $binRoot '.gdignore'), '')
[System.IO.File]::WriteAllText((Join-Path $testRoot 'logs/.gdignore'), '')
$consoleGodot = Join-Path (Split-Path -Parent $GodotPath) (([System.IO.Path]::GetFileNameWithoutExtension($GodotPath)) + '_console.exe')
if (Test-Path -LiteralPath $consoleGodot) { $GodotPath = $consoleGodot }
if (-not $FmodRuntime) {
    $FmodRuntime = Join-Path $repoRoot 'src/thirdparty/fmod/lib/x64/fmod.dll'
}
if (-not (Test-Path -LiteralPath $FmodRuntime)) {
    throw 'FMOD runtime missing. Pass -FmodRuntime with your locally installed SDK fmod.dll path.'
}
foreach ($extension in @('.dll', '.pdb')) {
    $fileName = 'fmod_player.windows.template_debug.x86_64' + $extension
    Sync-TestFile (Join-Path $repoRoot ('addons/fmod_player/bin/' + $fileName)) (Join-Path $binRoot $fileName)
}
Sync-TestFile $FmodRuntime (Join-Path $binRoot 'fmod.dll')
Push-Location $repoRoot
try {
    if ($Import -or -not (Test-Path -LiteralPath (Join-Path $testRoot '.godot/extension_list.cfg'))) {
        & $GodotPath --headless --path $testRoot --editor --import res://main.tscn --log-file (Join-Path $logRoot 'import.log') *> (Join-Path $logRoot 'import-console.log')
        if ($LASTEXITCODE -ne 0) { throw "Test project import failed; see $logRoot" }
    }
    if (-not $PrepareOnly) {
        $testArguments = @('--headless', '--path', $testRoot, '--log-file', (Join-Path $logRoot 'godot.log'), '--', "--suite=$Suite", "--output-dir=$logRoot")
        if ($Quick) { $testArguments += '--quick' }
        & $GodotPath @testArguments *> (Join-Path $logRoot 'console.log')
        if ($LASTEXITCODE -ne 0) { throw "Tests failed; see $logRoot" }
        if (-not (Select-String -LiteralPath (Join-Path $logRoot 'godot.log') -SimpleMatch 'TESTS_PASSED' -Quiet)) {
            throw "Test completion marker missing; see $logRoot"
        }
    }
    Write-Output "Test output: $logRoot"
} finally {
    Pop-Location
}
