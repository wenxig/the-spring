# the-spring 四月天

## Using Vite+, the Unified Toolchain for the Web

This project is using Vite+, a unified toolchain built on top of Vite, Rolldown, Vitest, tsdown, Oxlint, Oxfmt, and Vite Task. Vite+ wraps runtime management, package management, and frontend tooling in a single global CLI called `vp`. Vite+ is distinct from Vite, and it invokes Vite through `vp dev` and `vp build`. Run `vp help` to print a list of commands and `vp <command> --help` for information about a specific command.

Docs are local at `node_modules/vite-plus/docs` or online at <https://viteplus.dev/guide/>.

### Review Checklist

- [ ] Run `vp install` after pulling remote changes and before getting started.
- [ ] Run `vp check` and `vp test` to format, lint, type check and test changes.
- [ ] Check if there are `vite.config.ts` tasks or `package.json` scripts necessary for validation, run via `vp run <script>`.
- [ ] Python 模块检查：在对应子包运行 `uv run --with ruff ruff check .` 与 `uv run --with pytest pytest`，或执行子包内定义之 `check` / `test` 脚本。
- [ ] C++ 模块检查：全链路采用 clang / cpp26 标准；执行 `clang-tidy`、`clang-format --dry-run --Werror` 及对应 CMake / CTest 目标完成静态检查与单测。
- [ ] If setup, runtime, or package-manager behavior looks wrong, run `vp env doctor` and include its output when asking for help.

### Notice

When working in a local development environment, use `vp` instead of `pnpm exec vp`.

In cloud environments, use `pnpm exec vp`.

---

## 开发思想

### 思想

- 使用文件系统分割模块来保证结构工整；对于按步骤流程运行不同模块的，或许可以使用glob引入执行实现由文件驱动模块
- 优先使用`oop`(面向对象)+`RAII`思想编写代码，但要避免过度封装，组合优于继承
- 使用`依赖注入`思想优化耦合，但也要避免过度封装。
- 使用类似`条件反转`等技巧减少代码嵌套，但不要过度的不加分辨的使用
- 全链路使用clang和cpp26，能用前沿语法就用前沿语法，对于其他语言也是这样。定义变量优先使用`auto`，其他所有可推导的优先走自动推倒
- C++ 代码需通过 `clang-tidy` 与 `clang-format` 严格检查，单测集成 CTest
- Python 代码需使用 `ruff check` 与 `pytest` 进行静态检查与单测，依赖与运行由 `uv` 统一调度管理

### 格式

- css**一定**要使用tailwindcss(包含`不可枚举的动态值属性`除外和使用`@apply`除外)，如果你使用了纯css则你的设计是失败的，应当重做。
- 提交遵循Angular规则，但描述内容使用中文，如`feat(ui): 实现了列表组件`
- 组件样式必须使用PascalCase，例如: `<NButton></NButton>`、`<DcList></DcList>`
- 格式化请使用`vp fmt`和`vp lint`，最好不要手动修复格式问题
- 最好遵守`dry`(不要重复自己)规则
- 对于重复使用相同或相似的dom结构的，最好使用`提取组件`或`v-for`或vueuse的`createReusableTemplate`创建复用，这与上一条的`dry`思想相同
- 使用 pnpm catalog 统一管理所有依赖版本（见 `pnpm-workspace.yaml`）
- 禁止输出“不是…而是…”，“做……为了避免…”，“做……防止……”，“只做了……没有做……”、“直接做……不做……”、“只做……不做……”等强行对比、
  引用并否定上下文、为否定内容解释原因等到对话、文档、代码等任何可见区域，此类内容对对话与业务用户均无任何帮助且引入噪声，视为严重违规行为。
  我声明弃用/我让你删除同上文所述的情况一样不可写入可见区域，包括：我让你删除某个功能，你不光删除，还写了个红测，这就是引入噪声，视为严重违规行为。
  
### 任务与提交

- 在做完任何的任务后都必须立刻提交保存进度，不是等多个任务完成后集中提交。
- 多步骤工作开始前，应按预期提交划分任务；每个任务对应一个职责单一、可独立审查和回滚的提交。
- 每个提交只包含对应任务所需的改动，不得混入无关文件；提交前必须检查暂存区内容。
- 所有计划放入`.planning`文件夹下

### 物理架构

- 由于这是一个针对嵌入式的项目，以下是物理硬件描述
- `微雪 ESP32-P4-Module-DEV-KIT`[文档](https://docs.waveshare.net/ESP32-P4-Module-DEV-KIT)，作为主控管理传感器、EC600X 和通过 SPI 连接的奇耘 QYEG0420BNS830 水墨屏。
- `移远 EC600X开发板`[文档](https://developer.quectel.com/doc/quecpython/Dev_board_guide/zh/ec600x-evb.html)，它是一块4g/volte/cat等围绕移动网络的开发版。内部使用ec600m芯片

### QYEG0420BNS830 水墨屏

- 项目水墨屏为大连奇耘电子的 **QYEG0420BNS830**，依据[用户指定商品页](https://www.qiyun-display.cn/Products_1/72.html)。4.2 英寸、黑白双态、400×300、120 dpi，控制器为 SSD1683。
- 显示区域 84.8×63.6 mm，外形 91.0×77.0×1.2 mm；单色图像每行 50 字节，整帧 15,000 字节。
- 面板工作电压 2.3–3.6 V，工作温度 0–50 ℃，储存温度 -25–70 ℃。项目采用 3.3 V 逻辑域，供电入口和外围电路按实际驱动板版本核对。
- 商品页支持全刷和局刷，标称全刷 4 s、局刷 0.6 s、刷新功耗 12.6 mW；实测条件、峰值电流和局刷维护阈值需进一步确认。
- ESP32-P4 的 C++ 显示服务负责 SPI、复位、BUSY 超时、图像缓冲、刷新策略和休眠恢复。局刷建立在有效旧帧基准上；刷新失败或掉电后重新建立全帧状态。
- 涉及本屏的接线、驱动、图像转换或调试时，读取项目技能 [.agents/skills/qyeg0420bns830/SKILL.md](.agents/skills/qyeg0420bns830/SKILL.md)。
- 商品页所附 `QYEG0420BNS830F0_V2.0.pdf` 与示例工程是针序、复位时序、LUT、VCOM 和初始化参数的优先依据。编写技能时附件返回 HTTP 403，具体 FPC/转接板针序和 ESP32 GPIO 映射保持待核对，连接前按随货资料确认。

### EC600M 控制边界

- EC600M 作为 ESP32-P4 的通信扩展使用，主控程序通过 UART 发送 AT 命令并解析响应和 URC。
- 网络注册、数据拨号、信号查询、基站定位信息和 VoLTE 控制均由 ESP32-P4 的 C++ 服务封装；EC600M 保持 modem 固件运行。
- 禁止为 EC600M 新增 QuecPython 应用、Raw REPL 上传流程或依赖 QuecPython 运行时的部署脚本。
- UART 连接遵循 EC600X-EVB J5 的 TX0/RX0/GND 定义，并由主控负责超时、重试、URC 分发和状态机管理。

采用该边界是因为 EC600M 的公开用户开发路径以 QuecPython 或受限的 QuecOpen SDK 为主，macOS 缺少稳定的 Quectel 串口驱动；AT modem 接口由 ESP32-P4 统一管理后，编译、烧录和调试链路保持在主控工程内，EC600M 只承担蜂窝通信能力。

### ESP32-P4 与 EC600X-EVB 物理连接

按当前两块开发板的排针定义，UART0 主串口采用交叉连接：

| ESP32-P4（40-Pin） | EC600X-EVB（J5） | 信号 |
| --- | --- | --- |
| Pin 1 / GPIO0（UART TX） | J5 Pin 7 / RX0 | ESP32 发送，EC600M 接收 |
| Pin 2 / GPIO1（UART RX） | J5 Pin 6 / TX0 | EC600M 发送，ESP32 接收 |
| Pin 3、8、13 或 18 / GND | J5 Pin 1、2 或 18 / GND | 公共信号地 |

两块开发板分别使用各自的 USB 供电，连接 UART 前先确认 EC600X-EVB 电源开关处于 USB 挡。J5 的 UART 信号经过 EVB 电平转换器并位于 3.3V 侧，可与 ESP32-P4 GPIO0/GPIO1 连接；J6 Pin 3（1.8V VDD_EXT）和 J6 Pin 18（约 3.8V VBAT）不得接入 ESP32 GPIO 或 3.3V 电源。不要把 EC600X-EVB 的 J6 Pin 1 5V 直接并接到 ESP32-P4 的 3V3_OUT。

PWRKEY、RESET_N、MAIN_RI 没有引出到 EC600X-EVB 的 J5/J6，首版连接通过板载 PWRKEY 和 RESET 按键完成开关机与复位。需要主控自动控制时，再按 EC600M 硬件手册增加晶体管或开漏下拉电路，并为 MAIN_RI 增加电平匹配输入；禁止把这些模组侧信号直接接到 ESP32 GPIO。

连接完成后的最小验证顺序是：ESP32 UART 发送 `AT`，等待 `OK`；查询 `AT+CPIN?`、`AT+CEREG?`、`AT+CSQ`，再执行数据拨号和 VoLTE 状态流程。USB 线仅用于分别烧录和日志查看，开发板之间的业务通信使用上述 UART 线。

### VoLTE 与 PCM 音频

VoLTE 的呼叫控制和音频承载分成两条链路：ESP32-P4 通过 UART 发送 `ATD<number>;`、`ATA`、`ATH` 并解析来电、接通和挂断 URC；通话语音由 EC600M 的音频接口承载。当前 EC600X-EVB 已把 EC600M 的模拟麦克风输入和差分扬声器输出接到板载 GMI6050P/NS4160，首版 VoLTE 使用板载麦克风和扬声器即可，ESP32 不需要传输 PCM 数据。

PCM/数字音频只有在产品需要 ESP32 处理语音、回声消除或外接 Codec 时才启用。现有 J5/J6 排针没有列出 PCM_CLK、PCM_SYNC、PCM_DIN、PCM_DOUT，不能从排针直接接线；必须依据 EC600M 硬件手册和 EVB 原理图确认模组焊盘、1.8V 电平、主从时钟和 Codec 连接，再设计电平转换及音频 Codec。ESP32-P4 的普通 I2S GPIO 不可直接当作 EC600M PCM 接口。

VoLTE 首版验收顺序：确认 `AT+CEREG?` 已注册、`AT+CSQ` 信号正常，使用 `ATD<number>;` 发起呼叫，监听 `VOICE CALL: BEGIN`/厂商对应 URC，确认板载扬声器和麦克风通话，再用 `ATH` 结束。具体音频通道、音量和 PCM 复用命令必须以 EC600M 当前固件 AT 手册为准，代码中集中封装并保留超时与失败回滚。

## 项目概览

采用 **pnpm monorepo** 架构。

- **线上仓库**: <https://github.com/wenxig/the-spring.git>
- **包位置**: 只有脚本子包才要放到`scripts`下，其他所有子包，无论什么语言编写，全部做成小repo放到`packages`下，项目根应当是清爽的
