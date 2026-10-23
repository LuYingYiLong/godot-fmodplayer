# FMOD 测试

以下命令在仓库根目录执行。

## 运行测试

```powershell
.\build_debug_to_test.bat
.\tools\run_tests.ps1 -Suite all
.\tools\run_tests.ps1 -Suite spectrum
.\tools\run_tests.ps1 -Suite capture
.\tools\run_tests.ps1 -Suite reverb
.\tools\run_tests.ps1 -Suite profile
```

构建脚本将完整插件复制到 `tests/addons/fmod_player`，并从本机 SDK 补入 `fmod.dll`。已有构建可用 `.\tools\run_tests.ps1 -PrepareOnly` 同步；自定义运行库路径用 `-FmodRuntime "SDK 路径/fmod.dll"`。

`-Quick` 用于快速检查，性能对比使用默认计时。`-Import` 刷新 Godot 导入缓存。

在 Godot 中打开 `tests/project.godot`，按 F5 运行自动测试；打开 `scenes/reverb_test.tscn`，按 F6 测试空间音频。

## 性能对比

1. 将旧版 DLL/PDB 备份到 `tests/logs/baseline_bin/`。
2. 运行旧版测试，修改代码并重新编译，再运行新版测试。
3. 用两次运行的实际目录生成对比报告。控制台会打印目录，`build.json` 记录 DLL/PDB 哈希。

前后使用相同脚本和编译参数。测试配置为 48 kHz、立体声、1024 帧/块，采用 `NOSOUND_NRT + STREAM_FROM_UPDATE`；计时包含基础混音和脚本开销，记录 5 次测量的中位数及范围。

当前测试使用 FMOD 2.04 封装 API。跨 API 版本对比时，旧 DLL 需搭配对应版本的测试脚本。

### Capture

```powershell
.\tools\run_tests.ps1 -Suite capture -CaptureBaseline -ExtensionBin "tests/logs/baseline_bin" -Label capture_before
# 修改代码后重新编译
.\build_debug_to_test.bat
.\tools\run_tests.ps1 -Suite capture -Label capture_after
python tools/compare_capture.py "tests/logs/<优化前目录>" "tests/logs/<优化后目录>"
```

`-CaptureBaseline` 用于旧版基线，允许已知的溢出容量、计数和声道错位检查失败。新版按完整回归检查，比较工具同时验证非溢出音频逐位一致。

### 混响

```powershell
.\tools\run_tests.ps1 -Suite reverb -ExtensionBin "tests/logs/baseline_bin" -Label reverb_before
# 修改代码后重新编译
.\build_debug_to_test.bat
.\tools\run_tests.ps1 -Suite reverb -Label reverb_after
python tools/compare_reverb.py "tests/logs/<优化前目录>" "tests/logs/<优化后目录>"
```

比较工具汇总耗时，并校验默认、高通、动态参数、尾音四组音频的逐位一致性。测试旧 DLL 后，用 `.\tools\run_tests.ps1 -PrepareOnly` 恢复当前构建。

## 日志与报告

| 内容 | 位置 |
|------|------|
| 命令行运行日志、计时、音频及构建哈希 | `tests/logs/<标签>_<时间>/` |
| F5 运行日志 | `tests/logs/godot.log` |
| F5 测试结果 | `tests/logs/run_<时间>/` |
| 优化构建日志 | `tests/logs/build_profile.log` |
| Capture 对比报告 | `tests/logs/capture_comparison.json` |
| 混响对比报告 | `tests/logs/reverb_comparison.json` |

日志、缓存和测试插件副本均由 Git 忽略。
