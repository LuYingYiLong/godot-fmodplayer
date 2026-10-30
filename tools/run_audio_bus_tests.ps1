param(
    [switch]$Editor,
    [switch]$SyncGodot,
    [switch]$Full,
    [string]$GodotPath = 'C:/Users/Administrator/Documents/Godot/Godot_v4.7.2-stable_win64_console.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$label = if ($Editor) { 'audio_buses_editor_' } else { 'audio_buses_runtime_' }
if ($SyncGodot) { $label += 'sync_' }
$run = Join-Path $root ('tests/logs/runs/' + $label + (Get-Date -Format 'yyyyMMdd_HHmmss_fff'))
$project = Join-Path $run 'project'
New-Item -ItemType Directory -Force -Path $project, (Join-Path $project 'addons'), (Join-Path $project '.godot') | Out-Null
foreach ($entry in Get-ChildItem -LiteralPath (Join-Path $root 'tests') -Force) {
    if ($entry.Name -notin @('addons', 'logs', '.godot', 'bin')) { Copy-Item -LiteralPath $entry.FullName -Destination $project -Recurse }
}
Copy-Item -LiteralPath (Join-Path $root 'addons/fmod_player') -Destination (Join-Path $project 'addons/fmod_player') -Recurse
Copy-Item -LiteralPath (Join-Path $root 'src/thirdparty/fmod/lib/x64/fmod.dll') -Destination (Join-Path $project 'addons/fmod_player/bin/fmod.dll')
if (-not $SyncGodot) { Copy-Item -LiteralPath (Join-Path $root 'tests/assets/mixer/default_fmod_bus_layout.tres') -Destination (Join-Path $project 'default_fmod_bus_layout.tres') }
$syncValue = if ($SyncGodot) { 'true' } else { 'false' }
$config = @"
config_version=5
[application]
config/name="FMOD Audio Buses Check"
config/features=PackedStringArray("4.7", "GL Compatibility")
[audio]
fmod/output_type=4
fmod/show_startup_banner=false
fmod/sync_godot_audio_buses=$syncValue
[editor_plugins]
enabled=PackedStringArray("res://addons/fmod_player/plugin.cfg")
[rendering]
renderer/rendering_method="gl_compatibility"
"@
$utf8 = [Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText((Join-Path $project 'project.godot'), $config, $utf8)
[IO.File]::WriteAllText((Join-Path $project '.godot/extension_list.cfg'), "res://addons/fmod_player/bin/fmod_player.gdextension`n", $utf8)
& $GodotPath --headless --editor --import --path $project --log-file (Join-Path $run 'import.log') *> (Join-Path $run 'import-console.log')
if ($LASTEXITCODE -ne 0) { throw "Import failed: $run" }
if ($Editor) {
    $config = $config.Replace('PackedStringArray("res://addons/fmod_player/plugin.cfg")', 'PackedStringArray("res://addons/fmod_player/plugin.cfg", "res://editor/plugin.cfg")')
    [IO.File]::WriteAllText((Join-Path $project 'project.godot'), $config, $utf8)
    if (-not $Full) {
        $runner = Join-Path $project 'editor/property_translation_test.gd'
        $text = [IO.File]::ReadAllText($runner)
        $text = [regex]::Replace($text, '(?m)^\tpassed = await (?!BUS_DOCK_TEST)[A-Z_]+\.new\(\)\.run\(output_dir, tree\) and passed\r?\n', '')
        [IO.File]::WriteAllText($runner, $text, $utf8)
    }
    $arguments = @('--editor', '--path', $project, '--position', '-32000,-32000', '--log-file', (Join-Path $run 'godot.log'), '--', "--output-dir=$run", '--eq-capture')
    $process = Start-Process -FilePath $GodotPath -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $run 'console.log') -RedirectStandardError (Join-Path $run 'stderr.log')
    $code = $process.ExitCode
} else {
    $suite = if ($Full) { 'regression' } else { 'routing' }
    $ErrorActionPreference = 'Continue'
    & $GodotPath --headless --path $project res://runner/automated.tscn --log-file (Join-Path $run 'godot.log') -- "--suite=$suite" "--output-dir=$run" *> (Join-Path $run 'console.log')
    $code = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
}
if ($code -ne 0) { throw "Audio bus checks failed ($code): $run" }
$logs = @((Join-Path $run 'console.log'))
if ($Editor) { $logs += Join-Path $run 'stderr.log' }
if (Select-String -LiteralPath $logs -Pattern '^(SCRIPT ERROR:|ERROR:)' -Quiet) { throw "Godot reported an error: $run" }
if (-not (Select-String -LiteralPath (Join-Path $run 'godot.log') -SimpleMatch 'TESTS_PASSED' -Quiet)) { throw "Completion marker missing: $run" }
Write-Output "Audio bus checks passed: $run"
