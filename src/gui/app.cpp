#include "app.h"
#include "../cli.h"
#include "../parser.h"
#include "../classifier.h"
#include "../validator.h"
#include "../excel_writer.h"
#include "../config.h"
#include "../exceptions.h"
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <wx/utils.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/scrolwin.h>
#include <wx/datetime.h>
#include <fstream>
#include <sstream>

namespace xingda {

// ============================================================
// 事件表
// ============================================================
wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_BUTTON(wxID_ANY, MainFrame::OnStartProcessing)
    EVT_MENU(wxID_ANY, MainFrame::OnStartProcessing) // fallback
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(wxEVT_PROCESSING_LOG, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_PROCESSING_DONE, wxThreadEvent);

// ============================================================
// PDF 拖拽目标
// ============================================================

PdfDropTarget::PdfDropTarget(MainFrame* frame)
    : m_frame(frame) {}

bool PdfDropTarget::OnDropFiles(wxCoord, wxCoord, const wxArrayString& filenames) {
    if (filenames.IsEmpty()) return false;
    wxString fname = filenames[0];
    if (fname.Lower().EndsWith(".pdf")) {
        m_frame->OnFileDropped(fname);
        return true;
    }
    return false;
}

// ============================================================
// 后台处理线程
// ============================================================

ProcessingThread::ProcessingThread(MainFrame* frame, const std::string& pdf_path,
                                   const std::string& output_dir,
                                   const std::optional<std::string>& rules_path,
                                   bool validate_only, bool dump_text,
                                   bool include_summary, bool enable_ocr,
                                   bool summary_only, bool no_merge,
                                   const std::optional<std::string>& style_name)
    : wxThread(wxTHREAD_DETACHED)
    , m_frame(frame)
    , m_pdf_path(pdf_path)
    , m_output_dir(output_dir)
    , m_rules_path(rules_path)
    , m_validate_only(validate_only)
    , m_dump_text(dump_text)
    , m_include_summary(include_summary)
    , m_enable_ocr(enable_ocr)
    , m_summary_only(summary_only)
    , m_no_merge(no_merge)
    , m_style_name(style_name) {}

wxThread::ExitCode ProcessingThread::Entry() {
    auto send_log = [this](const wxString& msg, LogTag tag = LogTag::Info) {
        auto* event = new wxThreadEvent(wxEVT_PROCESSING_LOG);
        event->SetString(msg);
        event->SetInt(static_cast<int>(tag));
        wxQueueEvent(m_frame, event);
    };

    send_log("\ud83d\ude80 \u5f00\u59cb\u5904\u7406: " + wxString(m_pdf_path)); // "🚀 开始处理: "

    ParserConfig pconfig = ParserConfig::default_config();

    try {
        std::string output = process_pdf_core(
            m_pdf_path, m_output_dir, m_rules_path,
            m_validate_only, m_dump_text, m_include_summary,
            m_enable_ocr, m_summary_only, m_no_merge,
            pconfig, m_style_name
        );

        if (!output.empty()) {
            send_log("\u2705 \u5b8c\u6210\uff01\u8f93\u51fa\u6587\u4ef6: " + wxString(output), LogTag::Success); // "✅ 完成！输出文件: "
            auto* doneEvent = new wxThreadEvent(wxEVT_PROCESSING_DONE);
            doneEvent->SetString(wxString(output));
            doneEvent->SetInt(1); // success
            wxQueueEvent(m_frame, doneEvent);
        } else {
            send_log("\u26a0 \u5904\u7406\u5b8c\u6210\u4f46\u672a\u751f\u6210\u6587\u4ef6", LogTag::Warning); // "⚠ 处理完成但未生成文件"
            auto* doneEvent = new wxThreadEvent(wxEVT_PROCESSING_DONE);
            doneEvent->SetString("");
            doneEvent->SetInt(0);
            wxQueueEvent(m_frame, doneEvent);
        }
    } catch (const std::exception& e) {
        send_log("\u274c \u9519\u8bef: " + wxString(e.what()), LogTag::Error); // "❌ 错误: "
        auto* doneEvent = new wxThreadEvent(wxEVT_PROCESSING_DONE);
        doneEvent->SetString(wxString(e.what()));
        doneEvent->SetInt(0);
        wxQueueEvent(m_frame, doneEvent);
    }

    return nullptr;
}

// ============================================================
// 主窗口
// ============================================================

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(680, 620),
              wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL)
{
    SetMinSize(wxSize(600, 500));

    // 主面板
    wxPanel* mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // UI 构建
    BuildMenuBar();

    // --- 标题区域 ---
    auto headerBox = new wxBoxSizer(wxVERTICAL);
    auto* titleLabel = new wxStaticText(mainPanel, wxID_ANY,
        L"\u5174\u8fbe\u70bc\u94c1\u4fdd\u4ea7\u4e8b\u4e1a\u90e8 \u00b7 \u7ed3\u7b97\u5355\u660e\u7ec6\u5de5\u5177",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    wxFont titleFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"Microsoft YaHei");
    titleLabel->SetFont(titleFont);
    headerBox->Add(titleLabel, 0, wxEXPAND | wxALL, 5);

    auto* subtitleLabel = new wxStaticText(mainPanel, wxID_ANY,
        L"PDF \u7ed3\u7b97\u5355 \u2192 \u81ea\u52a8\u63d0\u53d6\u8003\u6838\u660e\u7ec6 \u2192 \u751f\u6210 Excel | v3.0.0",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    subtitleLabel->SetForegroundColour(wxColour(128, 128, 128));
    headerBox->Add(subtitleLabel, 0, wxEXPAND | wxALL, 2);

    auto* dropHint = new wxStaticText(mainPanel, wxID_ANY,
        L"\U0001f4c2 \u62d6\u62fd PDF \u6587\u4ef6\u5230\u7a97\u53e3\u4efb\u610f\u4f4d\u7f6e",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    dropHint->SetForegroundColour(wxColour(0, 120, 212));
    wxFont hintFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"Microsoft YaHei");
    dropHint->SetFont(hintFont);
    headerBox->Add(dropHint, 0, wxEXPAND | wxALL, 5);

    mainSizer->Add(headerBox, 0, wxEXPAND | wxALL, 10);

    // --- 文件选择区域 ---
    wxStaticBox* fileBox = new wxStaticBox(mainPanel, wxID_ANY, L"\u6587\u4ef6\u9009\u62e9"); // "文件选择"
    wxStaticBoxSizer* fileSizer = new wxStaticBoxSizer(fileBox, wxVERTICAL);

    // PDF 行
    auto* pdfRow = new wxBoxSizer(wxHORIZONTAL);
    pdfRow->Add(new wxStaticText(fileBox->GetStaticBox(), wxID_ANY, L"PDF \u6587\u4ef6\uff1a"), 0, wxALIGN_CENTER | wxRIGHT, 5);
    m_pdfPath = new wxTextCtrl(fileBox->GetStaticBox(), wxID_ANY, L"", wxDefaultPosition, wxSize(-1, -1));
    pdfRow->Add(m_pdfPath, 1, wxEXPAND | wxRIGHT, 5);
    auto* browsePdfBtn = new wxButton(fileBox->GetStaticBox(), wxID_ANY, L"\u6d4f\u89c8..."); // "浏览..."
    browsePdfBtn->Bind(wxEVT_BUTTON, &MainFrame::OnBrowsePdf, this);
    pdfRow->Add(browsePdfBtn, 0);
    fileSizer->Add(pdfRow, 0, wxEXPAND | wxBOTTOM, 8);

    // 输出目录行
    auto* outRow = new wxBoxSizer(wxHORIZONTAL);
    outRow->Add(new wxStaticText(fileBox->GetStaticBox(), wxID_ANY, L"\u8f93\u51fa\u76ee\u5f55\uff1a"), 0, wxALIGN_CENTER | wxRIGHT, 5);
    m_outputDir = new wxTextCtrl(fileBox->GetStaticBox(), wxID_ANY, L"./output", wxDefaultPosition, wxSize(-1, -1));
    outRow->Add(m_outputDir, 1, wxEXPAND | wxRIGHT, 5);
    auto* browseOutBtn = new wxButton(fileBox->GetStaticBox(), wxID_ANY, L"\u6d4f\u89c8...");
    browseOutBtn->Bind(wxEVT_BUTTON, &MainFrame::OnBrowseOutput, this);
    outRow->Add(browseOutBtn, 0);
    fileSizer->Add(outRow, 0, wxEXPAND | wxBOTTOM, 8);

    // 分类规则行
    auto* rulesRow = new wxBoxSizer(wxHORIZONTAL);
    rulesRow->Add(new wxStaticText(fileBox->GetStaticBox(), wxID_ANY, L"\u5206\u7c7b\u89c4\u5219\uff1a"), 0, wxALIGN_CENTER | wxRIGHT, 5);
    m_rulesPath = new wxTextCtrl(fileBox->GetStaticBox(), wxID_ANY, L"", wxDefaultPosition, wxSize(-1, -1));
    rulesRow->Add(m_rulesPath, 1, wxEXPAND | wxRIGHT, 5);
    auto* browseRulesBtn = new wxButton(fileBox->GetStaticBox(), wxID_ANY, L"\u6d4f\u89c8...");
    browseRulesBtn->Bind(wxEVT_BUTTON, &MainFrame::OnBrowseRules, this);
    rulesRow->Add(browseRulesBtn, 0);
    rulesRow->Add(new wxStaticText(fileBox->GetStaticBox(), wxID_ANY,
        L"\uff08\u53ef\u9009\uff0c\u9ed8\u8ba4\u4f7f\u7528 classify_rules.yaml\uff09"), 0, wxALIGN_CENTER);
    fileSizer->Add(rulesRow, 0, wxEXPAND);

    mainSizer->Add(fileSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 15);

    // --- 选项区域 ---
    wxStaticBox* optBox = new wxStaticBox(mainPanel, wxID_ANY, L"\u9009\u9879"); // "选项"
    wxStaticBoxSizer* optSizer = new wxStaticBoxSizer(optBox, wxHORIZONTAL);

    m_validateOnly = new wxCheckBox(optBox->GetStaticBox(), wxID_ANY,
        L"\u4ec5\u6821\u9a8c\u4e0d\u751f\u6210 Excel\uff08--validate-only\uff09");
    m_dumpText = new wxCheckBox(optBox->GetStaticBox(), wxID_ANY,
        L"\u540c\u65f6\u5bfc\u51fa\u539f\u59cb\u6587\u672c\uff08--dump-text\uff09");
    optSizer->Add(m_validateOnly, 0, wxALL, 5);
    optSizer->Add(m_dumpText, 0, wxALL, 5);

    mainSizer->Add(optSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 15);

    // --- 按钮行 ---
    auto* btnRow = new wxBoxSizer(wxHORIZONTAL);

    m_processBtn = new wxButton(mainPanel, wxID_ANY, L"\u25b6  \u5f00\u59cb\u5904\u7406", wxDefaultPosition, wxSize(130, 30));
    m_processBtn->Bind(wxEVT_BUTTON, &MainFrame::OnStartProcessing, this);
    btnRow->Add(m_processBtn, 0, wxRIGHT, 10);

    auto* openDirBtn = new wxButton(mainPanel, wxID_ANY,
        L"\U0001f4c1 \u6253\u5f00\u8f93\u51fa\u76ee\u5f55", wxDefaultPosition, wxSize(140, 30));
    openDirBtn->Bind(wxEVT_BUTTON, &MainFrame::OnOpenOutputDir, this);
    btnRow->Add(openDirBtn, 0, wxRIGHT, 10);

    auto* clearBtn = new wxButton(mainPanel, wxID_ANY, L"\u6e05\u7a7a\u65e5\u5fd7", wxDefaultPosition, wxSize(80, 30));
    clearBtn->Bind(wxEVT_BUTTON, &MainFrame::OnClearLog, this);
    btnRow->Add(clearBtn, 0, wxRIGHT, 10);

    m_progress = new wxGauge(mainPanel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 20), wxGA_HORIZONTAL);
    btnRow->Add(m_progress, 1, wxALIGN_CENTER);
    m_progress->Hide();

    mainSizer->Add(btnRow, 0, wxEXPAND | wxALL, 10);

    // --- 日志区域 ---
    wxStaticBox* logBox = new wxStaticBox(mainPanel, wxID_ANY, L"\u5904\u7406\u65e5\u5fd7"); // "处理日志"
    wxStaticBoxSizer* logSizer = new wxStaticBoxSizer(logBox, wxVERTICAL);

    m_logText = new wxTextCtrl(logBox->GetStaticBox(), wxID_ANY, L"",
        wxDefaultPosition, wxSize(-1, 250),
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    m_logText->SetBackgroundColour(wxColour(30, 30, 30));
    m_logText->SetForegroundColour(wxColour(212, 212, 212));
    wxFont logFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    m_logText->SetFont(logFont);
    logSizer->Add(m_logText, 1, wxEXPAND);

    mainSizer->Add(logSizer, 1, wxEXPAND | wxALL, 15);

    // --- 状态栏 ---
    auto* statusRow = new wxBoxSizer(wxHORIZONTAL);
    m_statusLabel = new wxStaticText(mainPanel, wxID_ANY, L"\u5c31\u7eea"); // "就绪"
    m_statusLabel->SetForegroundColour(wxColour(128, 128, 128));
    statusRow->Add(m_statusLabel, 0, wxALIGN_CENTER);
    mainSizer->Add(statusRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);

    mainPanel->SetSizer(mainSizer);

    // 启用拖拽
    SetDropTarget(new PdfDropTarget(this));

    // 绑定线程事件
    Bind(wxEVT_PROCESSING_LOG, &MainFrame::OnProcessingLog, this);
    Bind(wxEVT_PROCESSING_DONE, &MainFrame::OnProcessingDone, this);

    // 首次运行日志
    wxString welcomeMsg = wxString::Format(
        L"\u2705 \u7a0b\u5e8f\u5df2\u542f\u52a8\uff0c\u7248\u672c v3.0.0\n"
        L"\u8bf7\u9009\u62e9 PDF \u7ed3\u7b97\u5355\u6587\u4ef6\u5f00\u59cb\u5904\u7406");
    AppendLog(L"==================================================", LogTag::Header);
    AppendLog(L"  \u5174\u8fbe\u70bc\u94c1\u4fdd\u4ea7\u4e8b\u4e1a\u90e8 \u00b7 \u7ed3\u7b97\u5355\u660e\u7ec6\u5de5\u5177", LogTag::Header);
    AppendLog(L"  PDF \u7ed3\u7b97\u5355 \u2192 \u63d0\u53d6\u8003\u6838\u660e\u7ec6 \u2192 \u751f\u6210 Excel", LogTag::Info);
    AppendLog(L"==================================================", LogTag::Header);
    AppendLog(L"", LogTag::Info);
    AppendLog(welcomeMsg, LogTag::Success);
    AppendLog(L"", LogTag::Info);
    AppendLog(L"\U0001f4a1 \u63d0\u793a\uff1aPDF \u6587\u4ef6\u53ef\u4ee5\u76f4\u63a5\u62d6\u62fd\u5230\u7a97\u53e3", LogTag::Info);
}

MainFrame::~MainFrame() {}

// ============================================================
// UI 构建
// ============================================================

void MainFrame::BuildMenuBar() {
    wxMenuBar* menubar = new wxMenuBar();

    // 文件菜单
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(1001, L"\u9009\u62e9 PDF \u6587\u4ef6...\tCtrl+O");
    fileMenu->Append(1002, L"\u9009\u62e9\u8f93\u51fa\u76ee\u5f55...");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, L"\u9000\u51fa\tCtrl+Q");
    menubar->Append(fileMenu, L"\u6587\u4ef6");

    // 工具菜单
    wxMenu* toolMenu = new wxMenu();
    toolMenu->Append(2001, L"\u73af\u5883\u68c0\u6d4b");
    toolMenu->Append(2002, L"\u5b89\u88c5\u7f3a\u5931\u4f9d\u8d56");
    menubar->Append(toolMenu, L"\u5de5\u5177");

    // 帮助菜单
    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, L"\u5173\u4e8e");
    menubar->Append(helpMenu, L"\u5e2e\u52a9");

    SetMenuBar(menubar);

    // 绑定菜单事件
    Bind(wxEVT_MENU, &MainFrame::OnBrowsePdf, this, 1001);
    Bind(wxEVT_MENU, &MainFrame::OnBrowseOutput, this, 1002);
    Bind(wxEVT_MENU, &MainFrame::OnQuit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainFrame::OnEnvCheck, this, 2001);
    Bind(wxEVT_MENU, &MainFrame::OnInstallDeps, this, 2002);
    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);

    // 快捷键
    wxAcceleratorEntry entries[2];
    entries[0].Set(wxACCEL_CTRL, 'O', 1001);
    entries[1].Set(wxACCEL_CTRL, 'Q', wxID_EXIT);
    wxAcceleratorTable accel(2, entries);
    SetAcceleratorTable(accel);
}


// ============================================================
// 事件处理
// ============================================================

void MainFrame::OnBrowsePdf(wxCommandEvent&) {
    wxFileDialog dlg(this, L"\u9009\u62e9\u7ed3\u7b97\u5355 PDF \u6587\u4ef6", // "选择结算单 PDF 文件"
        wxEmptyString, wxEmptyString,
        L"PDF \u6587\u4ef6 (*.pdf)|*.pdf|\u6240\u6709\u6587\u4ef6 (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
        m_pdfPath->SetValue(dlg.GetPath());
        wxString fname = wxFileName(dlg.GetPath()).GetFullName();
        AppendLog(L"\U0001f4c4 \u5df2\u9009\u62e9: " + fname, LogTag::Info); // "📄 已选择: "
    }
}

void MainFrame::OnBrowseOutput(wxCommandEvent&) {
    wxDirDialog dlg(this, L"\u9009\u62e9\u8f93\u51fa\u76ee\u5f55");
    if (dlg.ShowModal() == wxID_OK) {
        m_outputDir->SetValue(dlg.GetPath());
    }
}

void MainFrame::OnBrowseRules(wxCommandEvent&) {
    wxFileDialog dlg(this, L"\u9009\u62e9\u5206\u7c7b\u89c4\u5219\u914d\u7f6e\u6587\u4ef6",
        wxEmptyString, wxEmptyString,
        L"YAML \u6587\u4ef6 (*.yaml;*.yml)|*.yaml;*.yml|\u6240\u6709\u6587\u4ef6 (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
        m_rulesPath->SetValue(dlg.GetPath());
    }
}

void MainFrame::OnStartProcessing(wxCommandEvent&) {
    if (m_processing) {
        AppendLog(L"\u26a0 \u6b63\u5728\u5904\u7406\u4e2d\uff0c\u8bf7\u7a0d\u5019...", LogTag::Warning); // "⚠ 正在处理中，请稍候..."
        return;
    }

    wxString pdfPath = m_pdfPath->GetValue().Trim();
    if (pdfPath.IsEmpty()) {
        wxMessageBox(L"\u8bf7\u5148\u9009\u62e9 PDF \u6587\u4ef6", L"\u63d0\u793a", wxOK | wxICON_WARNING); // "请先选择 PDF 文件"
        return;
    }
    if (!wxFileName::FileExists(pdfPath)) {
        wxMessageBox(L"\u6587\u4ef6\u4e0d\u5b58\u5728:\n" + pdfPath, L"\u9519\u8bef", wxOK | wxICON_ERROR); // "文件不存在"
        return;
    }

    m_processing = true;
    m_processBtn->Enable(false);
    m_processBtn->SetLabel(L"\u23f3 \u5904\u7406\u4e2d..."); // "⏳ 处理中..."
    m_progress->Show();
    m_progress->Pulse();
    m_statusLabel->SetLabel(L"\u5904\u7406\u4e2d..."); // "处理中..."
    m_statusLabel->SetForegroundColour(wxColour(0, 120, 212));
    m_logText->Clear();

    std::string pdfStr = pdfPath.ToStdString();
    std::string outDir = m_outputDir->GetValue().Trim().ToStdString();
    std::optional<std::string> rulesPath;
    wxString rp = m_rulesPath->GetValue().Trim();
    if (!rp.IsEmpty()) rulesPath = rp.ToStdString();

    auto* thread = new ProcessingThread(this, pdfStr, outDir, rulesPath,
        m_validateOnly->GetValue(), m_dumpText->GetValue(),
        true, false, false, false, std::nullopt);
    thread->Run();
}

void MainFrame::OnOpenOutputDir(wxCommandEvent&) {
    wxString outDir = m_outputDir->GetValue().Trim();
    if (outDir.IsEmpty()) outDir = L"./output";
    if (!wxDirExists(outDir)) {
        wxFileName::Mkdir(outDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
    wxLaunchDefaultBrowser(wxString(L"file:///") + wxFileName(outDir).GetFullPath());
    AppendLog(L"\U0001f4c2 \u5df2\u6253\u5f00\u8f93\u51fa\u76ee\u5f55: " + outDir, LogTag::Info);
}

void MainFrame::OnClearLog(wxCommandEvent&) {
    m_logText->Clear();
}

void MainFrame::OnEnvCheck(wxCommandEvent&) {
    m_logText->Clear();
    AppendLog(L"==================================================", LogTag::Header);
    AppendLog(L"\U0001f50d \u73af\u5883\u68c0\u6d4b", LogTag::Header); // "🔍 环境检测"
    AppendLog(L"==================================================", LogTag::Header);
    AppendLog(L"", LogTag::Info);
    AppendLog(L"\U0001f40d C++ \u7248\u672c\uff0c\u65e0\u989d\u5916 Python \u4f9d\u8d56", LogTag::Success);
    AppendLog(L"\u2705 \u6838\u5fc3\u529f\u80fd\u5df2\u5185\u5d4c\uff0c\u65e0\u9700\u989d\u5916\u5b89\u88c5", LogTag::Success);
    AppendLog(L"", LogTag::Info);
    AppendLog(L"\u26a0 \u82e5\u9700 OCR \u529f\u80fd\uff0c\u8bf7\u786e\u4fdd\u5df2\u5b89\u88c5:", LogTag::Warning);
    AppendLog(L"  - Tesseract OCR (https://github.com/UB-Mannheim/tesseract/wiki)", LogTag::Info);
    AppendLog(L"  - Poppler / pdftoppm (https://github.com/oschwartz10612/poppler-windows/releases/)", LogTag::Info);
}

void MainFrame::OnInstallDeps(wxCommandEvent&) {
    AppendLog(L"\U0001f4e5 \u68c0\u6d4b\u4f9d\u8d56...", LogTag::Info);
    AppendLog(L"\u2705 C++ \u7248\u672c\u65e0\u9700\u5b89\u88c5 Python \u4f9d\u8d56\uff0c\u76f4\u63a5\u8fd0\u884c\u5373\u53ef", LogTag::Success);
}

void MainFrame::OnAbout(wxCommandEvent&) {
    wxMessageBox(
        L"\u5174\u8fbe\u70bc\u94c1\u4fdd\u4ea7\u4e8b\u4e1a\u90e8\n"
        L"PDF\u7ed3\u7b97\u5355\u8f6cExcel\u660e\u7ec6\u5de5\u5177\n\n"
        L"\u7248\u672c: v3.0.0 (C++)\n"
        L"\u5b8c\u5168\u9002\u914d Windows 7 x64\n\n"
        L"\u81ea\u52a8\u8bfb\u53d6\u7532\u65b9\u7ed3\u7b97\u5355PDF\u6587\u4ef6\uff0c\n"
        L"\u63d0\u53d6\u8003\u6838\u4e8b\u9879\u660e\u7ec6\u6570\u636e\uff0c\n"
        L"\u6309\u4f5c\u4e1a\u533a\u5206\u7c7b\u5e76\u6309\u8003\u6838\u91d1\u989d\u964d\u5e8f\u6392\u5217\uff0c\n"
        L"\u751f\u6210\u683c\u5f0f\u5316\u7684Excel\u660e\u7ec6\u6587\u4ef6\u3002\n\n"
        L"\u4e8b\u4e1a\u90e8\u8003\u6838\u91d1\u989d = \u7532\u65b9\u8003\u6838\u91d1\u989d \u00d7 1%",
        L"\u5173\u4e8e", wxOK | wxICON_INFORMATION);
}

void MainFrame::OnQuit(wxCommandEvent&) {
    Close(true);
}

// ============================================================
// 线程事件
// ============================================================

void MainFrame::OnProcessingLog(wxThreadEvent& event) {
    wxString msg = event.GetString();
    LogTag tag = static_cast<LogTag>(event.GetInt());
    AppendLog(msg, tag);
}

void MainFrame::OnProcessingDone(wxThreadEvent& event) {
    m_processing = false;
    m_processBtn->Enable(true);
    m_processBtn->SetLabel(L"\u25b6  \u5f00\u59cb\u5904\u7406"); // "▶  开始处理"
    m_progress->Hide();

    wxString result = event.GetString();
    bool success = event.GetInt() != 0;

    if (success) {
        m_statusLabel->SetLabel(L"\u2705 \u5904\u7406\u5b8c\u6210"); // "✅ 处理完成"
        m_statusLabel->SetForegroundColour(wxColour(106, 153, 85));
        AppendLog(L"", LogTag::Info);
        AppendLog(L"==================================================", LogTag::Header);
        AppendLog(L"\U0001f4c1 \u8f93\u51fa\u6587\u4ef6: " + result, LogTag::Success);
        AppendLog(L"\u70b9\u51fb\u300c\u6253\u5f00\u8f93\u51fa\u76ee\u5f55\u300d\u67e5\u770b\u6587\u4ef6", LogTag::Info);

        // 自动打开输出目录
        wxCommandEvent dummy;
        OnOpenOutputDir(dummy);
    } else {
        m_statusLabel->SetLabel(L"\u274c \u5904\u7406\u5931\u8d25"); // "❌ 处理失败"
        m_statusLabel->SetForegroundColour(wxColour(244, 71, 71));
    }
}

// ============================================================
// 辅助函数
// ============================================================

void MainFrame::AppendLog(const wxString& msg, LogTag tag) {
    m_logText->SetDefaultStyle(wxTextAttr(wxColour(212, 212, 212)));
    
    switch (tag) {
        case LogTag::Success:
            m_logText->SetDefaultStyle(wxTextAttr(wxColour(106, 153, 85)));
            break;
        case LogTag::Warning:
            m_logText->SetDefaultStyle(wxTextAttr(wxColour(206, 145, 120)));
            break;
        case LogTag::Error:
            m_logText->SetDefaultStyle(wxTextAttr(wxColour(244, 71, 71)));
            break;
        case LogTag::Header:
            m_logText->SetDefaultStyle(wxTextAttr(wxColour(86, 156, 214)));
            break;
        default:
            break;
    }

    m_logText->AppendText(msg + L"\n");
}

void MainFrame::OnFileDropped(const wxString& filepath) {
    m_pdfPath->SetValue(filepath);
    wxString fname = wxFileName(filepath).GetFullName();
    AppendLog(L"\U0001f4e5 \u62d6\u5165\u6587\u4ef6: " + fname, LogTag::Info); // "📥 拖入文件: "
}

// ============================================================
// GUI 应用入口
// ============================================================

bool XingDaApp::OnInit() {
    if (!wxApp::OnInit()) return false;

    // 设置应用程序名称
    SetAppName(L"XingDaLianTieJieSuan");
    SetAppDisplayName(L"\u5174\u8fbe\u70bc\u94c1\u4fdd\u4ea7\u4e8b\u4e1a\u90e8 \u7ed3\u7b97\u5355\u660e\u7ec6\u5de5\u5177");

    auto* frame = new MainFrame(
        L"\u5174\u8fbe\u70bc\u94c1\u4fdd\u4ea7\u4e8b\u4e1a\u90e8 \u7ed3\u7b97\u5355\u660e\u7ec6\u5de5\u5177 v3.0.0");
    frame->Show(true);
    return true;
}

} // namespace xingda