param([string]$GodotPath = 'C:/Users/Administrator/Documents/Godot/Godot_v4.7.2-stable_win64_console.exe')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$run = Join-Path $root ('tests/logs/runs/audio_buses_packaging_' + (Get-Date -Format 'yyyyMMdd_HHmmss_fff'))
$project = Join-Path $run 'project'
$utf8 = [Text.UTF8Encoding]::new($false)
New-Item -ItemType Directory -Force -Path (Join-Path $project 'addons'), (Join-Path $project '.godot'), (Join-Path $project 'assets/mixer'), (Join-Path $run 'export') | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'addons/fmod_player') -Destination (Join-Path $project 'addons/fmod_player') -Recurse
Copy-Item -LiteralPath (Join-Path $root 'src/thirdparty/fmod/lib/x64/fmod.dll') -Destination (Join-Path $project 'addons/fmod_player/bin/fmod.dll')
Copy-Item -LiteralPath (Join-Path $root 'tests/assets/mixer/bus_eq.tres') -Destination (Join-Path $project 'assets/mixer/bus_eq.tres')
Copy-Item -LiteralPath (Join-Path $root 'tests/assets/mixer/default_fmod_bus_layout.tres') -Destination (Join-Path $project 'default_fmod_bus_layout.tres')
Copy-Item -LiteralPath (Join-Path $root 'tests/regression/audio_bus_startup_test.gd') -Destination (Join-Path $project 'verify.gd')
[IO.File]::WriteAllText((Join-Path $project '.godot/extension_list.cfg'), "res://addons/fmod_player/bin/fmod_player.gdextension`n", $utf8)
$config = @'
config_version=5
[application]
config/name="FMOD bus packaging checks"
config/features=PackedStringArray("4.7", "GL Compatibility")
[audio]
fmod/output_type=4
fmod/show_startup_banner=false
fmod/sync_godot_audio_buses=false
[editor_plugins]
enabled=PackedStringArray("res://addons/fmod_player/plugin.cfg")
'@
[IO.File]::WriteAllText((Join-Path $project 'project.godot'), $config, $utf8)
[IO.File]::WriteAllText((Join-Path $project 'assets/unselected.tres'), "[gd_resource type=`"Resource`" format=3]`n[resource]`n", $utf8)
[IO.File]::WriteAllText((Join-Path $project 'selected.tscn'), "[gd_scene format=3]`n[node name=`"Selected`" type=`"Node`"]`n", $utf8)
function Invoke-Check([string]$Label, [string[]]$Arguments) {
    $ErrorActionPreference = 'Continue'
    & $GodotPath @Arguments *> (Join-Path $run ($Label + '.log'))
    $result = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($result -ne 0) { throw "$Label failed ($result): $run" }
    if (Select-String -LiteralPath (Join-Path $run ($Label + '.log')) -Pattern '^(SCRIPT ERROR:|ERROR:)' -Quiet) { throw "$Label reported an error: $run" }
}
Invoke-Check 'import' @('--headless', '--editor', '--import', '--path', $project)
Invoke-Check 'valid' @('--headless', '--path', $project, '--script', 'res://verify.gd', '--', '--startup-mode=valid', "--output-dir=$run")
# Give the layout a stable UID, move it, then let the editor rebuild its UID map.
$layoutPath = Join-Path $project 'default_fmod_bus_layout.tres'
$layoutText = [IO.File]::ReadAllText($layoutPath)
$layoutText = $layoutText.Replace('format=3]', 'format=3 uid="uid://bq8wev4f5ax27"]')
[IO.File]::WriteAllText($layoutPath, $layoutText, $utf8)
Move-Item -LiteralPath $layoutPath -Destination (Join-Path $project 'moved_layout.tres')
$uidConfig = $config.Replace('fmod/sync_godot_audio_buses=false', "fmod/sync_godot_audio_buses=false`nfmod/default_bus_layout=`"uid://bq8wev4f5ax27`"")
[IO.File]::WriteAllText((Join-Path $project 'project.godot'), $uidConfig, $utf8)
Invoke-Check 'uid-import' @('--headless', '--editor', '--import', '--path', $project)
Invoke-Check 'uid-moved' @('--headless', '--path', $project, '--script', 'res://verify.gd', '--', '--startup-mode=uid', "--output-dir=$run")
$preset = @'
[preset.0]
name="FMOD selective"
platform="Windows Desktop"
runnable=true
export_filter="scenes"
export_files=PackedStringArray("res://selected.tscn")
include_filter="verify.gd,*.gdextension"
exclude_filter=""
export_path=""
[preset.0.options]
binary_format/architecture="x86_64"
'@
[IO.File]::WriteAllText((Join-Path $project 'export_presets.cfg'), $preset, $utf8)
$pack = Join-Path $run 'export/buses.pck'
Invoke-Check 'selective-export' @('--headless', '--editor', '--path', $project, '--export-pack', 'FMOD selective', $pack)
# Pack-only export omits native shared objects. Stage the matching local files
# beside the PCK, as a full executable export does through Godot's exporter.
New-Item -ItemType Directory -Path (Join-Path $run 'export/addons/fmod_player/bin') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $project 'addons/fmod_player/bin/fmod_player.windows.template_debug.x86_64.dll'), (Join-Path $project 'addons/fmod_player/bin/fmod.dll') -Destination (Join-Path $run 'export/addons/fmod_player/bin')
# Run the pack from an empty directory so loose project files cannot satisfy dependencies.
$probe = Join-Path $run 'probe'
New-Item -ItemType Directory -Path $probe | Out-Null
Push-Location $probe
try { Invoke-Check 'packed-startup' @('--headless', '--path', (Join-Path $run 'export'), '--main-pack', $pack, '--script', 'res://verify.gd', '--', '--startup-mode=pack', "--output-dir=$run") }
finally { Pop-Location }
foreach ($mode in @('missing', 'invalid')) {
    $caseConfig = $config.Replace('fmod/sync_godot_audio_buses=false', "fmod/sync_godot_audio_buses=false`nfmod/default_bus_layout=`"res://$mode.tres`"")
    [IO.File]::WriteAllText((Join-Path $project 'project.godot'), $caseConfig, $utf8)
    if ($mode -eq 'invalid') { [IO.File]::WriteAllText((Join-Path $project 'invalid.tres'), "[gd_resource type=`"FmodAudioBusLayout`" format=3]`n[resource]`nlayout_version=999`n", $utf8) }
    Invoke-Check $mode @('--headless', '--path', $project, '--script', 'res://verify.gd', '--', "--startup-mode=$mode", "--output-dir=$run")
}
Write-Output "Audio bus packaging checks passed: $run"
