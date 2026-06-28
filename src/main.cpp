#include <wx/wx.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <optional>

#include "cli.h"
#include "config.h"

#ifdef _WIN32
// Windows 7 兼容性定义
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif
#include <windows.h>
#endif

// ============================================================
// wxWidgets 应用 - XingDaApp 在 gui/app.h 中声明，gui/app.cpp 中实现
// ============================================================

// 前向声明：XingDaApp 在 namespace xingda 中定义
namespace xingda {
    class XingDaApp;
}

// wxIMPLEMENT_APP_NO_MAIN 需要类定义的可见性
// XingDaApp 在 gui/app.cpp 中的定义满足要求
#include "gui/app.h"
wxIMPLEMENT_APP_NO_MAIN(xingda::XingDaApp);

// ============================================================
// WinMain 入口（Windows）/ main 入口（其他平台）
// ============================================================

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    
    // 检查是否为命令行模式（有参数）
    std::string args;
    if (lpCmdLine && std::strlen(lpCmdLine) > 0) {
        args = lpCmdLine;
    }
    
    if (!args.empty()) {
        // CLI 模式：解析参数并处理
        std::string pdf_path;
        std::string output_dir = "./output";
        bool validate_only = false;
        bool dump_text = false;
        bool no_summary = false;
        bool summary_only = false;
        bool enable_ocr = false;
        
        std::vector<std::string> tokens;
        std::istringstream iss(args);
        std::string token;
        while (iss >> token) {
            if (token.size() >= 2 && token.front() == '"' && token.back() == '"') {
                token = token.substr(1, token.size() - 2);
            }
            tokens.push_back(token);
        }
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i] == "-o" || tokens[i] == "--output") {
                if (i + 1 < tokens.size()) output_dir = tokens[++i];
            } else if (tokens[i] == "--validate-only") {
                validate_only = true;
            } else if (tokens[i] == "--dump-text") {
                dump_text = true;
            } else if (tokens[i] == "--no-summary") {
                no_summary = true;
            } else if (tokens[i] == "--summary-only") {
                summary_only = true;
            } else if (tokens[i] == "--enable-ocr") {
                enable_ocr = true;
            } else if (tokens[i] == "-h" || tokens[i] == "--help") {
                std::fprintf(stdout,
                    "\u5174\u8fbe\u70bc\u94c1\u4fdd\u4ea7\u4e8b\u4e1a\u90e8 - PDF\u7ed3\u7b97\u5355\u8f6cExcel\u660e\u7ec6\u5de5\u5177 v3.0.0 (C++)\n\n"
                    "\u7528\u6cd5: XingDaLianTieJieSuan.exe [\u9009\u9879] <PDF\u6587\u4ef6>\n\n"
                    "\u9009\u9879:\n"
                    "  -o, --output <dir>     \u6307\u5b9a\u8f93\u51fa\u76ee\u5f55\uff08\u9ed8\u8ba4: ./output\uff09\n"
                    "  --validate-only         \u4ec5\u6821\u9a8c\u4e0d\u751f\u6210 Excel\n"
                    "  --dump-text             \u5bfc\u51fa\u539f\u59cb\u6587\u672c\n"
                    "  --no-summary            \u4e0d\u751f\u6210\u6c47\u603b\u4fe1\u606f\u533a\u57df\n"
                    "  --summary-only          \u4ec5\u751f\u6210\u6c47\u603b sheet\n"
                    "  --enable-ocr            \u542f\u7528 OCR\uff08\u9700\u8981 Tesseract + Poppler\uff09\n"
                    "  -h, --help              \u663e\u793a\u5e2e\u52a9\n"
                    "\n"
                    "\u65e0\u53c2\u6570\u542f\u52a8\u5c06\u6253\u5f00\u56fe\u5f62\u5316\u754c\u9762\u3002\n"
                );
                return 0;
            } else if (!tokens[i].empty() && tokens[i][0] != '-') {
                pdf_path = tokens[i];
            }
        }
        
        if (pdf_path.empty()) {
            std::fprintf(stderr, "\u9519\u8bef: \u672a\u6307\u5b9a PDF \u6587\u4ef6\u3002\u4f7f\u7528 -h \u67e5\u770b\u5e2e\u52a9\u3002\n");
            return 1;
        }
        
        try {
            xingda::process_pdf_core(
                pdf_path, output_dir, std::nullopt,
                validate_only, dump_text, !no_summary,
                enable_ocr, summary_only, false,
                xingda::ParserConfig::default_config(), std::nullopt
            );
            std::fprintf(stdout, "\u5904\u7406\u5b8c\u6210\uff01\u8f93\u51fa\u76ee\u5f55: %s\n", output_dir.c_str());
            return 0;
        } catch (const std::exception& e) {
            std::fprintf(stderr, "\u9519\u8bef: %s\n", e.what());
            return 1;
        }
    }
    
    // GUI 模式
    wxEntryStart(hInstance, nullptr, nullptr, nCmdShow);
    if (wxTheApp) {
        if (wxTheApp->OnInit()) {
            int ret = wxTheApp->OnRun();
            wxTheApp->OnExit();
            wxEntryCleanup();
            return ret;
        }
    }
    wxEntryCleanup();
    return 1;
}
#else
int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string pdf_path;
        std::string output_dir = "./output";
        bool validate_only = false;
        bool dump_text = false;
        bool no_summary = false;
        bool summary_only = false;
        bool enable_ocr = false;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg(argv[i]);
            if (arg == "-o" || arg == "--output") {
                if (i + 1 < argc) output_dir = argv[++i];
            } else if (arg == "--validate-only") {
                validate_only = true;
            } else if (arg == "--dump-text") {
                dump_text = true;
            } else if (arg == "--no-summary") {
                no_summary = true;
            } else if (arg == "--summary-only") {
                summary_only = true;
            } else if (arg == "--enable-ocr") {
                enable_ocr = true;
            } else if (arg == "-h" || arg == "--help") {
                std::printf("兴达炼铁保产事业部 - PDF结算单转Excel明细工具 v3.0.0 (C++)\n"
                       "用法: XingDaLianTieJieSuan [选项] <PDF文件>\n"
                       "无参数启动将打开图形化界面。\n");
                return 0;
            } else {
                pdf_path = arg;
            }
        }
        
        if (pdf_path.empty()) {
            std::fprintf(stderr, "错误: 未指定 PDF 文件\n");
            return 1;
        }
        
        try {
            xingda::process_pdf_core(
                pdf_path, output_dir, std::nullopt,
                validate_only, dump_text, !no_summary,
                enable_ocr, summary_only, false,
                xingda::ParserConfig::default_config(), std::nullopt
            );
            return 0;
        } catch (const std::exception& e) {
            std::fprintf(stderr, "错误: %s\n", e.what());
            return 1;
        }
    }
    
    return wxEntry(argc, argv);
}
#endif