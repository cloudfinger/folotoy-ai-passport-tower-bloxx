<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

Tower Bloxx 的暗色封面与游戏背景源图分别为 `images/tower-cover-source.png` 和
`images/tower-backdrop-source.png`；对应的日光版为
`images/tower-cover-light-source.png` 和 `images/tower-backdrop-light-source.png`。
四张生成图均为 1086 × 1448。安装 Pillow 后运行
`python3 tools/convert_tower_cover.py`，生成四张 `*-240x320.png` RGB565 屏幕预览，
以及 `main/` 下四个各 153,600 字节的小端 RGB565 资产（`tower_cover.rgb565`、
`tower_backdrop.rgb565`、`tower_cover_light.rgb565`、`tower_backdrop_light.rgb565`）。
它们由 `main/CMakeLists.txt` 嵌入 Flash，LVGL 直接读取，无需分配整屏 RAM。
来源：2026-09-13 使用 OpenAI 图像生成工具为本项目创作；日光版基于原图编辑，未使用第三方游戏美术。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。
