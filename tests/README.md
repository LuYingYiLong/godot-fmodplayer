# FMOD 测试

以下命令在仓库根目录执行。

## 手动测试

在 Godot 中打开 `tests/project.godot`，按 F5 进入测试菜单：

- 空间音频、混响与几何遮挡：FMOD 音源自动播放，WASD 移动，左键捕获鼠标，Esc 显示鼠标
- FMOD / Godot 2D 播放对比：用按钮选择单独播放、同时播放或停止
- 自动回归：启动独立离线进程，完成后在菜单显示结果

两个手动场景都可返回菜单，也可打开 `manual/reverb_test.tscn` 或 `manual/playback_compare.tscn` 按 F6 运行。两份 MP3 内容相同，分别保存 FMOD 和 Godot 导入设置，用于播放对比。

Panner 检查器：选中 `manual/panner_preview.tres`，左键点击或拖动上方声像图调节 `Pan`，也可使用下方原生参数。拖出左右边界会限制为 `-1` 和 `1`，支持撤销重做。视图使用编辑器主题颜色。

EQ 检查器：选中 `manual/eq6_preview.tres`、`eq10_preview.tres` 或 `eq21_preview.tres`。拖动频段控制点，横向调节频率，纵向调节峰值/搁架滤波的增益；低通、高通、带通、陷波和全通类型仅调节频率。支持撤销重做、只读状态和预设切换。曲线是根据滤波参数计算的近似响应，未使用实时频谱；打开预览不会创建额外音频 DSP。

## 自动测试

```powershell
.\build_debug_to_test.bat
.\tools\run_tests.ps1 -Suite regression
.\tools\run_tests.ps1 -Suite routing
.\tools\run_tests.ps1 -Suite spatial
.\tools\run_tests.ps1 -Suite smoke
.\tools\run_tests.ps1 -Suite spectrum
.\tools\run_tests.ps1 -Suite capture
.\tools\run_tests.ps1 -Suite reverb
.\tools\run_tests.ps1 -Suite profile
.\tools\run_tests.ps1 -Suite editor
python tools/run_native_tests.py
```

| 入口 | 内容 |
|------|------|
| `regression` | 3D 空间音频、混音矩阵、总线、SDK、频谱、Capture、混响正确性检查 |
| `routing` | 矩阵步长、边界、独奏绑定和总线父级回归 |
| `spatial` | 3D 滤波、禁用距离衰减、角度检查器元数据和场景保存回归 |
| `smoke` | SoundGroup 播放限制回归及 FMOD 2.04 API、效果器和音频资源冒烟检查 |
| `spectrum` | 频谱快照复用回归 |
| `capture` | Capture 回归和专项计时 |
| `reverb` | 混响音频、尾音回归和专项计时 |
| `profile` | DSP 效果器性能测量 |
| `all` | 保留原行为：全部回归加 DSP 性能测量 |
| `editor` | 在完整编辑器生命周期中检查属性翻译及 Panner、EQ 检查器 |
| `run_native_tests.py` | C++ 环形缓冲单/双精度检查及 EQ 预览响应模型检查 |

自动场景入口为 `runner/automated.tscn`，命令行脚本显式选择它。编辑器检查会临时启用 `editor/plugin.cfg`，结束后恢复项目配置。原生检查使用 MSVC 或 `CXX` 指定的 C++ 编译器。

`smoke`、`regression` 和 `all` 包含 SoundGroup 限制回归，检查方法及属性的 `-1` 取消限制、`0` 禁止可听播放、无效负数和实际并发播放。结果保存为本次日志目录中的 `sound_group_regression.json`。

`spatial` 使用启用了 `CHANNEL_DISTANCEFILTER` 的离线系统，保存双音 WAV、角度往返场景和 `spatial_audio_regression.json` 到本次日志目录。完整套件随后重建常规离线系统，其他回归和性能测量保持原有初始化参数。手动测试项目也已启用 `audio/fmod/init_flags/channel_distance_filter`；实际项目需启用此设置并重新运行，距离滤波才会生效。

构建脚本将完整插件复制到 `tests/addons/fmod_player`，并从本机 SDK 补入 `fmod.dll`。已有构建可用 `.\tools\run_tests.ps1 -PrepareOnly` 同步；`-Import` 刷新导入缓存。首次导入时脚本临时使用主场景资源路径，结束后恢复项目配置中的 UID。

通过 `-GodotPath "Godot 可执行文件路径"` 和 `-FmodRuntime "SDK 的 fmod.dll 路径"` 指定本机环境。`-Quick` 缩短性能测量时长，回归检查保持相同用例；性能对比使用默认计时。

Linux 使用以下命令，同样会同步完整插件并保存日志：

```sh
export GODOT_BIN=/path/to/Godot
export FMOD_LINUX_SDK=/path/to/fmodstudioapi20400linux
bash tools/run_tests_linux.sh --suite=regression
bash tools/run_tests_linux.sh --suite=profile --quick
bash tools/run_tests_linux.sh --suite=editor
FMOD_TEST_TARGET=template_release bash tools/run_tests_linux.sh --suite=regression
python3 tools/run_native_tests.py
```

先用 Debug 导入项目，再测 Release。Linux 脚本临时使用离线音频配置，结束后还原项目设置；Release 只在测试插件副本切换加载目标，默认命令可恢复 Debug。

## 目录

| 目录 | 用途 |
|------|------|
| `runner/` | 参数、离线音频配置、套件调度和自动场景入口 |
| `regression/` | 3D 空间音频、矩阵、总线、SDK、Capture、频谱和混响正确性检查 |
| `benchmarks/` | DSP、Capture 和混响性能测量 |
| `manual/` | 测试菜单、播放对比和空间音频场景 |
| `editor/` | 编辑器属性翻译测试插件 |
| `native/` | C++ 环形缓冲测试 |
| `assets/` | 音频素材、材质和 shader |
| `addons/` | 本地部署的完整插件副本 |

## 性能对比

1. 将旧版 DLL/PDB 备份到 `tests/logs/baseline_bin/`
2. 运行旧版测试，修改代码并重新编译，再运行新版测试
3. 用两次运行的实际目录生成对比报告，控制台会打印目录，`build.json` 记录 DLL/PDB 哈希

前后使用相同脚本和编译参数。测试采用 48 kHz、立体声、1024 帧/块和 `NOSOUND_NRT + STREAM_FROM_UPDATE`，记录 5 次测量的中位数及范围。计时包含基础混音和脚本开销。跨 FMOD 封装 API 版本对比时，旧 DLL 需搭配对应版本的脚本。

### Capture

```powershell
.\tools\run_tests.ps1 -Suite capture -CaptureBaseline -ExtensionBin "tests/logs/baseline_bin" -Label capture_before
# 修改代码后重新编译
.\build_debug_to_test.bat
.\tools\run_tests.ps1 -Suite capture -Label capture_after
python tools/compare_capture.py "tests/logs/runs/<优化前目录>" "tests/logs/runs/<优化后目录>"
```

`-CaptureBaseline` 允许旧版基线的已知溢出容量、计数和声道错位检查失败。新版执行完整回归，比较工具同时验证非溢出音频逐位一致。

### 混响

```powershell
.\tools\run_tests.ps1 -Suite reverb -ExtensionBin "tests/logs/baseline_bin" -Label reverb_before
# 修改代码后重新编译
.\build_debug_to_test.bat
.\tools\run_tests.ps1 -Suite reverb -Label reverb_after
python tools/compare_reverb.py "tests/logs/runs/<优化前目录>" "tests/logs/runs/<优化后目录>"
```

比较工具汇总耗时，并校验默认、高通、动态参数和尾音四组音频的逐位一致性。旧 DLL 测试结束后，用 `.\tools\run_tests.ps1 -PrepareOnly` 恢复当前构建。

## 日志与报告

| 内容 | 位置 |
|------|------|
| Godot、编辑器和原生测试日志与结果 | `logs/runs/<标签>_<时间>/` |
| 编译日志 | `logs/builds/` |
| Capture 和混响对比报告 | `logs/comparisons/` |
| F5 手动场景日志 | `logs/editor.log` |
| 历史运行与诊断资料 | `logs/archive/pre_reorganization/` |
| 历史资料迁移清单 | `logs/archive/reorganization_index.json` |

旧基线二进制目录保留原路径。文档迁移的临时源码与副本归档到仓库根目录 `.local/maintenance_archive/`。日志内容、缓存、本地维护归档和测试插件副本均由 Git 忽略；保留 `logs/.gdignore`，防止 Godot 导入日志和诊断资料。
