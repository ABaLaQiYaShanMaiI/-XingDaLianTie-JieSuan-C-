# 兴达炼铁保产事业部 — 结算单明细工具 (C++版)

> **版本**: v3.0.0 (C++) | **目标**: 完全适配 Windows 7 x64  
> **功能**: PDF 结算单 → 自动提取考核明细 → 按作业区分类 → 生成格式化 Excel

---

## 📋 项目目标

本工具用于 **兴达炼铁保产事业部** 的月度结算单处理工作流：

1. **输入**：甲方发来的 PDF 格式结算单（通常为文本型 PDF，偶尔为扫描件）
2. **处理**：自动提取"考核事项"表格——逐条解析序号、描述、条款依据和考核金额
3. **分类**：基于可配置的 YAML 规则，将每条记录归入对应的作业区（事业部 / 原料分厂 / 供矿 / 煤库）
4. **校验**：对程序提取的考核金额合计与 PDF 声明的合计进行闭环校验（偏差 <5% 自动通过）
5. **输出**：生成一张多区域的格式化 Excel 明细文件（汇总信息 + 各区域明细 = 事业部考核金额 × 1%）

---

## 🏗 项目结构

```
-XingDaLianTie-JieSuan-C++--main/
├── CMakeLists.txt              # CMake 构建配置（C++17, Win7 SDK）
├── classify_rules.yaml         # 分类规则配置（可自行修改区域/关键词）
├── README.md                   # 本文件
├── output/                     # 默认输出目录（自动创建）
├── scripts/                    # 辅助脚本
│   ├── setup_tools.bat         # 自动检测 & 配置外部 OCR 工具
│   ├── win7_test.bat           # Windows 7 兼容性测试脚本
│   └── win7_stress_test.bat    # Windows 7 压力测试脚本
└── src/
    ├── main.cpp                # 入口（无参数→GUI / 有参数→CLI）
    ├── models.h                # 数据模型：考核记录 / 作业区 / 结算单
    ├── exceptions.h            # 异常层次
    ├── config.h / config.cpp   # YAML 规则配置加载（自研解析器）
    ├── parser.h / parser.cpp   # PDF 文本提取 + 正则解析 + OCR 回退
    ├── classifier.h / classifier.cpp  # 规则驱动分类引擎
    ├── validator.h / validator.cpp    # 金额闭环校验
    ├── excel_writer.h / excel_writer.cpp  # 自包含 Excel XML 生成
    ├── cli.h / cli.cpp         # 核心处理管线（GUI 和 CLI 共用）
    └── gui/
        ├── app.h / app.cpp     # wxWidgets 图形化界面
```

---

## 🔧 依赖说明

### C++ 版与 Python / Rust 版的本质区别

| 项目 | 核心技术 | 依赖管理 |
|------|---------|---------|
| **Python 版** | tkinter + pdfplumber + openpyxl + PyYAML | 需安装 Python 3.8+ 及 6 个 pip 包 |
| **Rust 版** | eframe/egui + pdf-extract + rust_xlsxwriter + serde_yaml | 编译时静态链接，产物 ~8MB |
| **C++ 版（本项目）** | wxWidgets + 自研核心 | **核心功能零依赖**，仅 PDF 文本提取需外部工具 |

### 核心功能（零额外依赖）

以下功能**完全自包含**在 C++ 代码中，不需要任何外部库或运行时：

- ✅ **Excel 生成** — 使用 XML Spreadsheet 2003 格式，Excel / WPS 均可直接打开
- ✅ **YAML 配置解析** — 手写解析器，无需 `yaml-cpp` 等第三方库
- ✅ **分类引擎** — 4 种匹配规则（序号 / 关键词 / 设备前缀正则 / 描述正则）
- ✅ **金额校验** — 闭环验证 + 偏差百分比计算
- ✅ **GUI 界面** — 基于 wxWidgets 3.0，原生 Windows 外观，支持拖拽

### 需要外部工具的功能

以下功能依赖系统级别的命令行工具（**不是代码库依赖**）：

| 功能 | 所需工具 | 用途 | 必需性 |
|------|---------|------|--------|
| PDF 文本提取 | **pdftotext**（Poppler 套件） | 将 PDF 转为可读文本 | **必需**（核心功能） |
| OCR 文字识别 | **tesseract** + **pdftoppm** | 处理扫描版 / 图片型 PDF | 可选（仅图片 PDF 需要） |

> **说明**：这些是独立的命令行程序，不是链接库。程序通过 `popen()` 调用它们，就像你在 cmd 里执行命令一样。  
> 如果只需要处理**文本型 PDF**（有文字层的电子版 PDF），仅需安装 **pdftotext** 即可。  
> Excel 生成、YAML 解析、分类、校验等所有其他功能**完全不依赖任何第三方库**。

---

## 🚀 构建方法

### Windows 7 x64 目标环境

```bash
# 1. 安装前置依赖
#    - CMake 3.10+
#    - wxWidgets 3.0.5（需用 VS 编译为 x64，WINVER=0x0601）
#    - Poppler（pdftotext.exe / pdftoppm.exe）→ 放入 PATH 或 tools/ 目录
#    - Visual Studio 2017 / 2019 / 2022

# 2. 构建
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_CXX_FLAGS="/DWINVER=0x0601 /D_WIN32_WINNT=0x0601" ^
    -DwxWidgets_ROOT_DIR=C:/wxWidgets-3.0.5 ^
    -DwxWidgets_LIB_DIR=C:/wxWidgets-3.0.5/lib/vc_x64_lib
cmake --build . --config Release

# 3. 运行
bin\Release\XingDaLianTieJieSuan.exe                    # 图形界面
bin\Release\XingDaLianTieJieSuan.exe 结算单.pdf          # 命令行处理
```

### 让构建产物独立运行

将以下文件/目录与 `XingDaLianTieJieSuan.exe` 放在**同一目录**：

```
你的发布目录/
├── XingDaLianTieJieSuan.exe    # 主程序
├── classify_rules.yaml         # 分类规则
├── wxbase32u_vc_custom.dll     # wxWidgets 运行时 DLL
├── wxmsw32u_core_vc_custom.dll
└── tools/                      # （可选）OCR 工具目录
    ├── pdftotext.exe
    ├── pdftoppm.exe
    └── tesseract/
        ├── tesseract.exe
        └── tessdata/
```

---

## 📖 使用方法

### 图形界面（双击运行）

```
双击 XingDaLianTieJieSuan.exe
  → 选择 PDF 文件（支持拖拽到窗口）
  → 选择输出目录
  → 点击「▶ 开始处理」
  → 自动生成 Excel 并打开输出目录
```

### 命令行

```bash
# 基本用法
XingDaLianTieJieSuan.exe 结算单.pdf

# 指定输出目录
XingDaLianTieJieSuan.exe 结算单.pdf -o ./我的输出/

# 仅校验，不生成 Excel
XingDaLianTieJieSuan.exe 结算单.pdf --validate-only

# 导出原始文本（调试用）
XingDaLianTieJieSuan.exe 结算单.pdf --dump-text

# 使用自定义分类规则
XingDaLianTieJieSuan.exe 结算单.pdf --rules 我的规则.yaml

# 显示帮助
XingDaLianTieJieSuan.exe -h
```

### Excel 输出格式

生成的 Excel 包含以下区域：

1. **结算单汇总信息**：合同编号 / 名称 / 作业时间 / 费用汇总
2. **区域考核概览**：每个作业区的条数和考核金额小计
3. **各区域明细**（按顺序）：
   - 事业部
   - 原料分厂作业区
   - 供矿作业区
   - 煤库作业区
   - 未分类
4. **校验失败警告**：当程序提取金额与 PDF 声明合计偏差 ≥5% 时，末尾附加红字警告行

---

## 🔬 测试脚本

项目提供和 Rust 版一样的 **批处理测试脚本**，用于暴露问题、生成日志：

| 脚本 | 用途 |
|------|------|
| `scripts/setup_tools.bat` | 自动检测系统已安装的 pdftotext / tesseract 并复制到便携目录 |
| `scripts/win7_test.bat` | 对测试 PDF 执行完整的参数组合测试，生成独立日志和汇总报告 |
| `scripts/win7_stress_test.bat` | 连续重复处理压力测试，检测内存泄漏和稳定性 |

测试日志输出到 `test_logs/` 目录，每条测试用例一个独立日志文件，汇总报告为 `_TEST_REPORT.txt`。

---

## 🎯 与 Python / Rust 版的对齐度

| 功能 | Python | Rust | C++（本项目） |
|------|--------|------|-------------|
| 图形界面 | ✅ tkinter | ✅ egui | ✅ wxWidgets |
| PDF 拖拽 | ✅ tkinterdnd2 + WM_DROPFILES | ✅ egui dropped_files | ✅ wxFileDropTarget |
| 双通道 PDF 解析 | ✅ 表格+文本 | ✅ 文本+OCR | ✅ 文本+OCR 回退 |
| YAML 规则分类 | ✅ PyYAML | ✅ serde_yaml | ✅ 自研解析器 |
| 金额闭环校验 | ✅ | ✅ | ✅ |
| 多颜色日志 | ✅ tkinter tags | ✅ egui colored_label | ✅ wxTextAttr |
| 格式化 Excel | ✅ openpyxl | ✅ rust_xlsxwriter | ✅ 自包含 XML |
| 命令行模式 | ✅ argparse | ✅ clap | ✅ 手写参数解析 |
| 仅校验模式 | ✅ | ✅ | ✅ |
| 原始文本导出 | ✅ | ✅ | ✅ |
| 处理完自动打开目录 | ✅ os.startfile | ✅ open crate | ✅ wxLaunchDefaultBrowser |
| Win7 x64 适配 | ⚠ 需要 Python 3.8 | ✅ | ✅ **原生目标** |

---

## 📄 许可

MIT License

---

## 👤 作者

兴达炼铁保产事业部

---

> **核心目标**：做一个**不依赖 Python 运行时、不依赖 Rust 工具链**的独立 EXE，在 Windows 7 x64 上双击就能运行。  
> 唯一硬性需求是系统里有 `pdftotext.exe`（可从 Poppler for Windows 获取，约 2MB）。