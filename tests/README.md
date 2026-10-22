# FMOD 测试项目

在 Godot 中打开本目录的 `project.godot`，按 F5 运行。主场景会依次运行频谱回归、17 个效果器的 SDK 冒烟测试和性能测试，并显示结果。

首次使用先在仓库根目录执行：

```powershell
.\build_profile_windows.bat
.\tools\run_tests.ps1 -PrepareOnly
```

准备脚本将本机编译的 DLL、PDB 和私有 SDK 中的 `fmod.dll` 同步到被忽略的 `tests/bin`。运行库不随测试项目提交。如果 SDK 不在默认位置，可传入 `-FmodRuntime "你的 SDK/api/core/lib/x64/fmod.dll"`。

命令行也直接运行这个 Godot 项目的主场景：

```powershell
.\tools\run_tests.ps1 -Suite all
.\tools\run_tests.ps1 -Suite spectrum
.\tools\run_tests.ps1 -Suite profile -Label before
.\tools\run_tests.ps1 -Suite profile -Label after
```

`-Quick` 用于快速检查运行流程，正式性能对比应使用默认计时。`-Import` 可显式重建项目导入缓存，通常首次准备后不需要再次导入。

所有运行日志和计时 JSON 都保存在 `tests/logs/<标签>_<时间>/`；直接按 F5 的运行日志保存在 `tests/logs/godot.log`，结果保存在该目录下的 `run_<时间>/`。优化构建日志位于 `tests/logs/build_profile.log`。缓存、日志、采样报告和本机二进制均由 `.gitignore` 排除。

测试使用 48 kHz 立体声、1024 帧音频块、无声非实时输出和 `MIX_FROM_UPDATE`。计时包含基础混音、脚本调用和查询开销；每项记录 5 次测量的中位数及范围，不代表实际游戏的 CPU 占比或实时音频期限。

频谱回归检查同一音频块的重复读取、同一个 Godot 帧内多次混音后的刷新、强制刷新、FFT 窗口大小变化、移除和换总线，以及已知 440 Hz / 4 kHz 信号的频段映射。
