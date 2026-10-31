# FMOD 测试项目

在 Godot 4.7.2 或更新版本中打开 `tests/project.godot`，等待资源导入完成后按 **F5**。所有日常测试都从这个项目和菜单进入。

## 准备插件

项目使用 `tests/addons/fmod_player` 中的本地插件副本。可以将已构建的完整插件及对应平台的 FMOD 运行库放入该目录；源码开发时，也可以在仓库根目录构建并同步：

```sh
scons platform=windows target=template_debug copy_to_tests=yes
```

Linux 将 `platform` 改为 `linux`，并配置 FMOD Linux SDK。SDK 安装和构建配置见[在线文档](https://godot-fmodplayerdocs.readthedocs.io/en/latest/)。

更新正在使用的原生库前，请先关闭这个测试项目。构建完成后重新打开项目即可；无需运行额外的测试工具。

## 在界面中测试

| 菜单入口 | 操作与观察 |
|----------|------------|
| 空间音频、混响与几何遮挡 | FMOD 音源自动播放；WASD 移动，左键捕获鼠标，Esc 显示鼠标。改变与音源的距离、绕过遮挡物、进入混响区域，试听声音变化 |
| FMOD / Godot 2D 播放对比 | 两份 WAV 内容相同，分别通过 FMOD 和 Godot 导入。用按钮选择单独播放、同时播放或停止，观察两种播放器的行为 |
| 运行数值回归（8 项） | 点击后等待逐项结果。检查静默运行，不影响手动场景的音频配置 |
| 查看回归报告 | 打开最近一次运行的报告目录，查看 `summary.json`、各项报告及 `godot.log` |

两个试听场景都有“返回测试菜单”按钮，也可以在编辑器中打开对应场景按 **F6** 单独运行。

数值回归覆盖：3D 距离滤波、混音矩阵、总线路由与效果器、频谱快照、SoundGroup 播放限制、FMOD API 与资源加载、Capture 缓冲区及声道顺序、混响输出与尾音。失败时会停止后续检查，菜单显示已完成项目的结果。

## 在编辑器中检查

这些检查直接在 Godot 编辑器中进行，不需要运行额外脚本：

- **音频资源与播放器**：选择播放器的 `stream`，检查波形、展开试听、拖放和资源菜单
- **混音总线**：打开 FMOD Audio Dock，检查总线和效果器的拖放、路由、旁路、撤销重做，以及保存后重新打开的布局
- **效果器预览**：在文件系统面板选择 `manual/*_preview.tres`，调整原生参数或操作预览控件，检查二者同步和撤销重做
- **空间声学节点**：打开 `manual/reverb_test.tscn`，检查声源、混响区域和几何遮挡的 Gizmo
- **翻译与帮助**：切换编辑器语言，检查属性名、FMOD 菜单和内置类帮助

效果器预览资源包括：

| 资源 | 交互 |
|------|------|
| `panner_preview.tres` | 点击或拖动声像图调整 Pan |
| `eq6_preview.tres`、`eq10_preview.tres`、`eq21_preview.tres` | 拖动频段控制点；悬停滚轮调整 Q |
| `filter_preview.tres` | 横向拖动调整截止频率；支持的模式可纵向调节 resonance |
| `compressor_preview.tres` | 拖动拐点调整阈值，拖动方点调整压缩比 |
| `amplify_preview.tres` | 上下拖动增益条 |
| `pitch_shift_preview.tres` | 拖动或滚轮调整音高 |
| `stereo_enhance_preview.tres` | 点击或拖动雷达调整宽度 |
| `delay_preview.tres` | 拖动 Tap 节点调整时间和 Pan；Alt + 点击切换重叠节点 |
| `distortion_preview.tres`、`hard_limiter_preview.tres`、`phaser_preview.tres` | 调整曲线控制点，检查对应参数变化 |
| `chorus_preview.tres`、`reverb_preview.tres` | 修改参数，观察声部或空间示意变化 |
| `capture_preview.tres`、`spectrum_analyzer_preview.tres` | 挂载到编辑器内正在播放的总线，观察采样或频谱变化 |

## 目录与结果

- `manual/`：测试菜单、试听场景和效果器预览资源
- `regression/`：菜单调用的 8 组关键数值检查
- `runner/`：数值检查的内部运行入口，开发者无需手动传参
- `assets/`：音频、材质和 shader
- `addons/`：本地插件副本，由 Git 忽略
- `logs/runs/`：每次回归的结果；`logs/editor.log`：项目运行日志

日志和历史诊断资料均由 Git 忽略。`logs/.gdignore` 阻止 Godot 导入这些资料。
