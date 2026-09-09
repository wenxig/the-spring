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
- `微雪 ESP32-P4-Module-DEV-KIT`[文档](https://docs.waveshare.net/ESP32-P4-Module-DEV-KIT)，它作为主要的控制板统领传感器和`ec600x`以及一块通过spi连接的水墨屏
- `移远 EC600X开发板`[文档](https://developer.quectel.com/doc/quecpython/Dev_board_guide/zh/ec600x-evb.html)，它是一块4g/volte/cat等围绕移动网络的开发版。内部使用ec600m芯片

## 项目概览

采用 **pnpm monorepo** 架构。

- **线上仓库**: <https://github.com/wenxig/the-spring.git>
- **包位置**: 只有脚本子包才要放到`scripts`下，其他所有子包，无论什么语言编写，全部做成小repo放到`packages`下，项目根应当是清爽的
