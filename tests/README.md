# FMOD 测试

以下命令在仓库根目录执行。

## 手动测试

在 Godot 中打开 `tests/project.godot`，按 F5 进入测试菜单：

- 空间音频、混响与几何遮挡：FMOD 音源自动播放，WASD 移动，左键捕获鼠标，Esc 显示鼠标
- FMOD / Godot 2D 播放对比：用按钮选择单独播放、同时播放或停止
- 自动回归：启动独立离线进程，完成后在菜单显示结果

两个手动场景都可返回菜单，也可打开 `manual/reverb_test.tscn` 或 `manual/playback_compare.tscn` 按 F6 运行。两份 MP3 内容相同，分别保存 FMOD 和 Godot 导入设置，用于播放对比。

Stream 检查器：三个 `FmodAudioStreamPlayer` 节点的 `stream` 在原生资源按钮内显示波形和文件名。预览层忽略鼠标，点击、右键菜单和拖放由原生按钮处理；点击资源可展开试听面板。两处共用后台波形缓存，音频数据或解码设置变更后自动更新。编辑器回归覆盖原生 picker 对比、鼠标操作、PCM/MP3、反相立体声和长音频，截图与报告保存在 `logs/runs/`。

Panner 检查器：选中 `manual/panner_preview.tres`，左键点击或拖动上方声像图调节 `Pan`，也可使用下方原生参数。拖出左右边界会限制为 `-1` 和 `1`，支持撤销重做。视图使用编辑器主题颜色。

EQ 检查器：选中 `manual/eq6_preview.tres`、`eq10_preview.tres` 或 `eq21_preview.tres`。拖动频段控制点，横向调节频率，纵向调节峰值/搁架滤波的增益；低通、高通、带通、陷波和全通类型仅调节频率。悬停在控制点上滚动滚轮调节 Q：向上增大，向下减小，每格约 10%，范围 `0.01–10`。支持撤销重做、只读状态和预设切换。曲线是根据滤波参数计算的近似响应，未使用实时频谱；打开预览不会创建额外音频 DSP。

Filter 检查器：选中 `manual/filter_preview.tres`，节点固定在 0 dB 参考线上，横向拖动调节截止频率，12/18/24 dB 模式可相对按下时的值纵向调节 resonance。直接点击或纯横向拖动会保留原 resonance，包括小于 0.1 的值。图表只保留 0 dB 参考线，使用编辑器主题颜色，最小高度为 160。曲线为近似响应，18 dB 按当前实现绘制为 24 dB；低通不使用 MULTIBAND_EQ 的 gain 参数。支持撤销重做与只读状态。

Compressor 检查器：选中 `manual/compressor_preview.tres`，左右拖动圆形拐点调阈值，上下拖动右侧方点调压缩比（向下增大）。阈值范围 `-60–0 dB`，压缩比范围 `1:1–48:1`，支持撤销重做和只读状态。曲线是包含 Gain、Mix 的静态硬拐点近似，Attack/Release 的时间响应不在图中显示；Mix 为 0 时仍可拖动调节压缩比。最小高度为 160，颜色取自编辑器主题。

Capture 检查器：选中 `manual/capture_preview.tres`，显示时间刻度和每段未读立体声数据的最小/最大振幅。预览读取当前编辑器进程中的 Capture 实例，未挂载资源显示空缓冲时间轴；F5 的独立运行进程拥有独立缓冲。修改 `buffer_length` 会调整时间范围并清空数据。视图最小高度 160，仅可见时以 10 Hz 刷新；快照复用缓存，读取位置及计数保持不变。编辑器测试会生成离线音频并保存实测预览截图。

Amplify 检查器：选中 `manual/amplify_preview.tres`，上下拖动连续填充的竖向增益条，范围 `-80–+24 dB`，右侧显示刻度。当前转换函数在 `-60 dB` 及以下输出静音，此时增益条清空。支持撤销重做、只读状态和线性增益同步；视图使用编辑器主题颜色，最小高度 160，按参数变化重绘。

Chorus 检查器：选中 `manual/chorus_preview.tres`，查看一秒范围内的声部调制示意。纵向位置表示 Pan，Rate/Depth 决定波形，Delay 决定起点。L 位于右上、R 位于右下，底部显示 1–4 号声部图例。曲线默认使用紫、绿、黄、红，支持 `voice_1_color` 至 `voice_4_color` 主题颜色覆盖；其他颜色取自编辑器主题。修改原生参数可更新预览，最小高度 160，按需重绘。

Reverb 检查器：选中 `manual/reverb_preview.tres`，查看椭圆环线堆叠的立体空间示意。Room Size/Spread 调整体积和宽度，Predelay/Feedback 调整环线分布，Damping 控制远端淡出，Wet/Dry 控制环线和中心声源亮度。使用编辑器主题颜色，最小高度 160，几何缓存按参数变化更新。

PitchShift 检查器：选中 `manual/pitch_shift_preview.tres`，查看半圆音高仪表盘。中央显示半音（st），外圈显示八度（oct），默认范围 ±2 oct，超出时自动扩展。点击或拖动外圈调整音高；悬停在表盘内滚轮每格调整 1 st，Shift + 滚轮调整 0.1 st，范围跟随 `pitch_scale` 的 `0.01–16`。支持撤销重做和只读状态，使用编辑器主题颜色，最小高度 160，按需重绘。

Phaser 检查器：选中 `manual/phaser_preview.tres`，主曲线显示扫频中点处、原声与滤波支路混合后的幅频响应（左侧 ±36 dB）。半透明参考线显示带反馈的滤波支路相位（右侧 ±180°），中央为 0 dB / 0° 参考线。半透明矩形覆盖 `range_min_hz` 到 `range_max_hz`，拖动左右边缘可调整上下限，支持撤销重做和只读状态。Depth、Feedback 会改变输出响应；Rate 调整扫频速度，静态预览保持不变。使用编辑器主题颜色，最小高度 160，两条曲线共用响应计算并缓存。

StereoEnhance 检查器：选中 `manual/stereo_enhance_preview.tres`，查看扇形雷达参数示意。张角表示 Pan Pullout（0 为中心线，1 为 45°，4 为整个半圆），内弧位置表示 Time Pullout，填充深浅表示 Surround。点击或拖动雷达调整宽度，支持撤销重做和只读状态。L/R 位于左右上角，使用编辑器主题颜色，最小高度 160，几何按参数变化缓存。

Delay 检查器：选中 `manual/delay_preview.tres`，查看同心半圆环线与两路 Tap 节点。半径表示延迟时间，角度表示 Pan，亮度表示电平；拖动节点可同时编辑时间和 Pan，Alt + 点击可选择重叠的另一节点。启用 Feedback 后显示低通滤波前的重复间隔与电平衰减示意。时间范围自动扩展，悬停提示显示范围；拖动期间保持比例，支持撤销重做、只读状态和 Tap 开关。使用编辑器主题颜色，最小高度 160，环线按参数变化缓存。

Distortion 检查器：选中 `manual/distortion_preview.tres`，显示六种模式的静态输入/输出幅度曲线，整形公式与音频处理共用。纵向拖动调 Drive；Overdrive 模式调 Pre Gain；Shift + 拖动调 Pre Gain，Ctrl + 拖动调 Post Gain。纵轴自动适配输出范围，支持撤销重做和只读状态。Keep HF 控制音频分频；Oversample 保留现有存储行为。

HardLimiter 检查器：选中 `manual/hard_limiter_preview.tres`，显示包含 Pre Gain 和 Ceiling 的稳态限幅折线。上下拖动圆点调 Pre Gain、方点调 Ceiling，支持撤销重做和只读状态。Release 控制恢复时间，静态曲线显示稳态结果。

SpectrumAnalyzer 检查器：选中 `manual/spectrum_analyzer_preview.tres`，显示当前编辑器实例的左右声道 FFT。L 使用主题强调色，R 使用半透明文字色，顶部显示 Hz/bin 分辨率。未挂载资源显示空坐标轴；F5 运行进程有独立实例。预览复用已有快照，仅可见时以 10 Hz 刷新，保持音频路由不变。编辑器测试生成左右声道不同频率的实际 FFT 并保存截图。

除 Record 外，全部 18 个可实例化效果器类（包含 EQ 基类及 EQ6/10/21）均有预览。Record 保留原生检查器。预览最小高度统一为 160，使用编辑器主题颜色。

## 自动测试

总线专项在 `logs/runs/` 下创建隔离项目，覆盖布局、运行时和真实编辑器交互：

```powershell
.\tools\run_audio_bus_tests.ps1
.\tools\run_audio_bus_tests.ps1 -Full
.\tools\run_audio_bus_tests.ps1 -Editor
.\tools\run_audio_bus_tests.ps1 -Editor -SyncGodot
.\tools\run_audio_bus_packaging_tests.ps1
```

检查配置保存重载、稳定 ID、自由路由和环路拒绝、删除转接、Solo 通路、共享配置的独立实例、参数更新与混响尾音、Dock 控件、翻译和撤销重做。JSON、布局副本、截图和控制台日志保存在同次运行目录。`-Editor -Full` 同时运行全部检查器回归。

打包专项还检查默认文件缺失、无效版本、UID 移动，以及仅导出选定场景时的布局和外部效果器依赖。PCK 在独立目录启动，原生库按 Godot 的 pack-only 导出规则单独准备。

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
| `editor` | 属性翻译、全部效果器的预览覆盖、交互、保存、定时器生命周期及真实 FFT/限幅音频检查 |
| `run_native_tests.py` | C++ 环形缓冲单/双精度、Capture 快照与峰值、EQ/Filter/Compressor/Phaser 响应及失真整形逐位一致性检查 |

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
| `native/` | C++ 环形缓冲与预览响应模型测试 |
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
