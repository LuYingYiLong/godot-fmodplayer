# FMOD 测试项目

在 Godot 中打开本目录的 `project.godot`，按 F5 运行。主场景会依次运行频谱回归、17 个效果器的 SDK 冒烟测试和性能测试，并显示结果。

首次使用先在仓库根目录执行：

```powershell
.\build_debug_to_test.bat
```

该脚本编译 `platform=windows target=template_debug arch=x86_64`，保留性能优化和 PDB 符号，然后将整个 `addons/fmod_player/` 复制到 `tests/addons/fmod_player/`，包括插件配置、导入器入口、工具脚本、翻译、图标、扩展描述文件和编译产物，并刷新 Godot 导入缓存。`fmod.dll` 从本机私有 SDK 补入插件的 `bin/`。测试插件副本整体由 Git 忽略，运行库不随项目提交。

如果 SDK 不在默认位置，可执行 `.\build_debug_to_test.bat -FmodRuntime "你的 SDK/api/core/lib/x64/fmod.dll"`。已经编译好时，可用 `.\tools\run_tests.ps1 -PrepareOnly` 仅同步测试插件；`.\build_profile_windows.bat` 仍然只负责编译。

命令行也直接运行这个 Godot 项目的主场景：

```powershell
.\tools\run_tests.ps1 -Suite all
.\tools\run_tests.ps1 -Suite spectrum
.\tools\run_tests.ps1 -Suite capture -Label capture_after
.\tools\run_tests.ps1 -Suite reverb -Label reverb_before
.\tools\run_tests.ps1 -Suite reverb -Label reverb_after
.\tools\run_tests.ps1 -Suite profile -Label before
.\tools\run_tests.ps1 -Suite profile -Label after
```

`-Quick` 用于快速检查运行流程，正式性能对比应使用默认计时。`-Import` 可显式重建项目导入缓存，通常首次准备后不需要再次导入。

所有运行日志和计时 JSON 都保存在 `tests/logs/<标签>_<时间>/`；直接按 F5 的运行日志保存在 `tests/logs/godot.log`，结果保存在该目录下的 `run_<时间>/`。优化构建日志位于 `tests/logs/build_profile.log`。缓存、日志、采样报告和本机二进制均由 `.gitignore` 排除。

项目默认使用正常音频输出，可在检查器中试听 FMOD 音频。打开 `scenes/reverb_test.tscn` 按 F6 可测试空间音频；播放器已绑定 `bgm_fire_extinguisher_all.mp3`。墙体的直接遮挡为 1.0，玩家在墙后可能听不到直达声，绕过墙体再比较。

自动测试主场景在自己的运行进程中切换为 48 kHz 立体声、1024 帧音频块、无声非实时输出和 `MIX_FROM_UPDATE`，不修改项目设置。计时包含基础混音、脚本调用和查询开销；每项记录 5 次测量的中位数及范围，不代表实际游戏的 CPU 占比或实时音频期限。

频谱回归检查同一音频块的重复读取、同一个 Godot 帧内多次混音后的刷新、强制刷新、FFT 窗口大小变化、移除和换总线，以及已知 440 Hz / 4 kHz 信号的频段映射。

混响专用套件记录默认、高通和大房间三组耗时，并输出默认、高通、动态参数、暂停输入后尾音四组确定性 float32 音频。动态参数覆盖 spread 的增减、0/500 ms 预延迟、高通、阻尼、反馈和干湿切换；尾音还检查有限数值和能量衰减。`all` 套件也会运行这四组输出回归。

前后运行目录可用以下命令对比，报告仍写入 `tests/logs`。对比工具同时检查逐样本输出是否完全一致：

```powershell
python tools/compare_reverb.py tests/logs/<优化前目录> tests/logs/<优化后目录>
```

要交替测试保留的旧 DLL/PDB，可用 `-ExtensionBin <本机旧二进制目录>` 指定来源。每次运行的 `build.json` 保存实际 DLL/PDB 哈希，便于核对前后版本；测试完成后使用默认来源再次准备，以恢复当前优化版。

Capture 专用套件检查左右不同的确定性信号、分段读取、环绕、缓冲溢出后的声道对齐、完整帧丢弃计数、奇数帧容量、清空、调整容量、重新绑定、单声道复制、多声道截取和资源释放。非溢出输出写入 `capture_steady.f32`；`all` 套件也运行这些回归。

Capture 的缓冲现在按完整立体声帧计数，48 kHz 下 0.1 秒可保存 4800 帧。缓冲满时保留已有数据，丢弃无法容纳的新帧，`get_discarded_frames()` 按完整帧统计。

Capture 性能测试包含空载、持续溢出、只写入（每块清空以保持可写）、每块读取、每 8 块批量读取五种情况。前后对比使用相同脚本和编译选项，比较工具同时验证非溢出输出逐位一致，以及新版全部回归通过：

```powershell
.\tools\run_tests.ps1 -Suite capture -CaptureBaseline -ExtensionBin <旧 DLL/PDB 目录> -Label capture_before
.\tools\run_tests.ps1 -Suite capture -Label capture_after
python tools/compare_capture.py tests/logs/<优化前目录> tests/logs/<优化后目录>
```

`-CaptureBaseline` 只用于旧版基线，允许已知的溢出容量、计数和声道错位检查失败，其他检查仍必须通过。正常运行不传此参数。报告默认保存在 `tests/logs/capture_comparison.json`。底层帧缓冲另有 `fmod_capture_ring_test.cpp`，覆盖单声道、立体声、多声道和 20000 次随机队列操作，可分别编译 float/double 精度运行。
