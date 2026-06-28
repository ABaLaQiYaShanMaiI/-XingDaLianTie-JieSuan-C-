#ifndef GUI_APP_H
#define GUI_APP_H

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/dnd.h>
#include <wx/thread.h>
#include <wx/msgqueue.h>
#include <string>
#include <optional>
#include "../config.h"

namespace xingda {

/// 日志消息类型
enum class LogTag { Info, Success, Warning, Error, Header };

/// 处理线程事件
wxDECLARE_EVENT(wxEVT_PROCESSING_LOG, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_PROCESSING_DONE, wxThreadEvent);

/// PDF 拖拽目标
class PdfDropTarget : public wxFileDropTarget {
public:
    PdfDropTarget(class MainFrame* frame);
    bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override;
private:
    MainFrame* m_frame;
};

/// 后台处理线程
class ProcessingThread : public wxThread {
public:
    ProcessingThread(MainFrame* frame, const std::string& pdf_path,
                     const std::string& output_dir,
                     const std::optional<std::string>& rules_path,
                     bool validate_only, bool dump_text,
                     bool include_summary, bool enable_ocr,
                     bool summary_only, bool no_merge,
                     const std::optional<std::string>& style_name);
    ExitCode Entry() override;

private:
    MainFrame* m_frame;
    std::string m_pdf_path;
    std::string m_output_dir;
    std::optional<std::string> m_rules_path;
    bool m_validate_only;
    bool m_dump_text;
    bool m_include_summary;
    bool m_enable_ocr;
    bool m_summary_only;
    bool m_no_merge;
    std::optional<std::string> m_style_name;
};

/// 主窗口
class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);
    ~MainFrame();

    void OnLogMessage(const wxString& msg, LogTag tag);
    void OnProcessingDone(const wxString& result, bool success);

private:
    // UI 构建
    void BuildMenuBar();
    void BuildHeader();
    void BuildFileSection();
    void BuildOptionsSection();
    void BuildButtons();
    void BuildLogArea();
    void BuildStatusBar();

    // 事件处理
    void OnBrowsePdf(wxCommandEvent& event);
    void OnBrowseOutput(wxCommandEvent& event);
    void OnBrowseRules(wxCommandEvent& event);
    void OnStartProcessing(wxCommandEvent& event);
    void OnOpenOutputDir(wxCommandEvent& event);
    void OnClearLog(wxCommandEvent& event);
    void OnEnvCheck(wxCommandEvent& event);
    void OnInstallDeps(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);

    // 线程事件
    void OnProcessingLog(wxThreadEvent& event);
    void OnProcessingDone(wxThreadEvent& event);

    // 辅助函数
    void AppendLog(const wxString& msg, LogTag tag = LogTag::Info);
    void OnFileDropped(const wxString& filepath);

    // 控件
    wxTextCtrl* m_pdfPath;
    wxTextCtrl* m_outputDir;
    wxTextCtrl* m_rulesPath;
    wxCheckBox* m_validateOnly;
    wxCheckBox* m_dumpText;
    wxButton* m_processBtn;
    wxGauge* m_progress;
    wxStaticText* m_statusLabel;
    wxTextCtrl* m_logText;

    // 状态
    bool m_processing = false;
    bool m_firstRun = true;

    wxDECLARE_EVENT_TABLE();
};

/// GUI 应用
class XingDaApp : public wxApp {
public:
    bool OnInit() override;
};

} // namespace xingda

#endif // GUI_APP_H