<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# FoloToy AI Passport 叠叠高 Tower Bloxx

一层一层，搭出自己的城市天际线。方形楼层在屏幕上左右移动，抓准时机按 **OK**，让它落在大楼上。落得越准，楼就越稳；一旦失误，下一层会更难放。

![Tower Bloxx 深色封面](assets/images/tower-cover-240x320.png)

## 游戏画面

主题选项位于标题菜单。以下界面预览根据当前游戏布局与美术素材重绘，用于展示玩法；它们不是实机照片或从设备截取的画面。

| 画面 | 黄昏主题 | 日光主题 |
| --- | --- | --- |
| 标题与主题设置 | ![黄昏标题菜单，选中深色主题](assets/images/readme/tower-menu-dark.png) | ![日光标题菜单，选中浅色主题](assets/images/readme/tower-menu-light.png) |
| 叠楼玩法 | ![黄昏快速游戏，叠起三层方形楼层](assets/images/readme/tower-gameplay-dark.png) | ![日光快速游戏，显示 Perfect 判定](assets/images/readme/tower-gameplay-light.png) |
| 建造城市 | ![黄昏城市地图，已有完成的大楼](assets/images/readme/tower-city-dark.png) | ![日光城市地图，准备放置大楼](assets/images/readme/tower-city-light.png) |
| 结算画面 | ![黄昏 Game Over 画面](assets/images/readme/tower-result-dark.png) | ![日光 Tower Built 结算画面](assets/images/readme/tower-result-light.png) |

## 玩法

- **Quick Game（快速游戏）：** 不断叠楼，挑战更高分数。
- **City Mode（城市模式）：** 完成大楼后，把它放进 4 × 4 城市地图，逐步扩展天际线。
- **UP / DOWN：** 切换菜单选项及城市位置。
- **OK：** 选择选项或放下一层。
- **长按 OK：** 返回游戏标题画面。
- **Theme（主题）：** 在标题画面选择黄昏或日光风格；选择会自动保存。

游戏使用原创像素城市封面、与之呼应的游玩背景、循环芯片音乐，以及成功放楼、Perfect、失误和结算时的不同音效。开机经过简短加载画面后，会直接进入游戏。

本仓库包含完整游戏源代码和美术素材，基于 [FoloToy AI Passport](https://github.com/FoloToy/ai-passport) 项目开发。构建与校验方法见[工程指南](docs/development/engineering/build-and-test.zh_CN.md)。构建成功后，完整固件位于 `build/tower-bloxx-full.bin`；生成的固件文件不会纳入 Git。源代码仓库采用 [MIT 许可证](LICENSE)。
