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
.\tools\run_tests.ps1 -Suite reverb -Label reverb_before
.\tools\run_tests.ps1 -Suite reverb -Label reverb_after
.\tools\run_tests.ps1 -Suite profile -Label before
.\tools\run_tests.ps1 -Suite profile -Label after
```

`-Quick` 用于快速检查运行流程，正式性能对比应使用默认计时。`-Import` 可显式重建项目导入缓存，通常首次准备后不需要再次导入。

所有运行日志和计时 JSON 都保存在 `tests/logs/<标签>_<时间>/`；直接按 F5 的运行日志保存在 `tests/logs/godot.log`，结果保存在该目录下的 `run_<时间>/`。优化构建日志位于 `tests/logs/build_profile.log`。缓存、日志、采样报告和本机二进制均由 `.gitignore` 排除。

测试使用 48 kHz 立体声、1024 帧音频块、无声非实时输出和 `MIX_FROM_UPDATE`。计时包含基础混音、脚本调用和查询开销；每项记录 5 次测量的中位数及范围，不代表实际游戏的 CPU 占比或实时音频期限。

频谱回归检查同一音频块的重复读取、同一个 Godot 帧内多次混音后的刷新、强制刷新、FFT 窗口大小变化、移除和换总线，以及已知 440 Hz / 4 kHz 信号的频段映射。

混响专用套件记录默认、高通和大房间三组耗时，并输出默认、高通、动态参数、暂停输入后尾音四组确定性 float32 音频。动态参数覆盖 spread 的增减、0/500 ms 预延迟、高通、阻尼、反馈和干湿切换；尾音还检查有限数值和能量衰减。`all` 套件也会运行这四组输出回归。

前后运行目录可用以下命令对比，报告仍写入 `tests/logs`。对比工具同时检查逐样本输出是否完全一致：

```powershell
python tools/compare_reverb.py tests/logs/<优化前目录> tests/logs/<优化后目录>
```

要交替测试保留的旧 DLL/PDB，可用 `-ExtensionBin <本机旧二进制目录>` 指定来源。每次运行的 `build.json` 保存实际 DLL/PDB 哈希，便于核对前后版本；测试完成后使用默认来源再次准备，以恢复当前优化版。
