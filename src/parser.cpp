#include "parser.h"
#include "exceptions.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <cstdio>
#include <array>
#include <memory>
#include <set>

#ifndef _WIN32
#include <cstdlib>
#endif

namespace xingda {

namespace fs = std::filesystem;

// ============================================================
// 配置常量
// ============================================================
static const int MAX_ITEM_INDEX = 99;
static const double MIN_ASSESSMENT_AMOUNT = 1.0;
static const int REWARD_SCAN_LINES = 5;
static const double REWARD_FILTER_THRESHOLD = 10.0;

// ============================================================
// 正则模式（与 Python 版对齐）
// ============================================================

// 考核记录描述前缀（中文月份/年份/季度）
static const char* ASSESSMENT_DESC_PREFIX_PAT =
    R"(\d+\s*-\s*\d+\s*月|)"       // "6 - 7月"
    R"(\d+\s*月|)"                    // "4月"
    R"(近\s*\d+\s*年|)"               // "近3年"
    R"(\d{4}\s*年|)"                  // "2025年"
    R"([一二三四五六七八九十]+\s*季度)"; // "三季度"

// 考核记录行模式
static std::regex RECORD_LINE_RE(
    std::string(R"(^(\d{1,2})\s+()") + ASSESSMENT_DESC_PREFIX_PAT + ")",
    std::regex::ECMAScript
);

// 多条款模式
static std::vector<std::regex> CLAUSE_PATTERNS = {
    std::regex(R"(炼铁厂.*?(?:条款|办法).*?(?:\d+\.\d+)?)", std::regex::ECMAScript),
    std::regex(R"(协力供应商.*?(?:标准|条款).*?(?:\d+\.\d+)?)", std::regex::ECMAScript),
    std::regex(R"(检修协力.*?(?:标准|条款).*?(?:\d+\.\d+)?)", std::regex::ECMAScript),
    std::regex(R"(协力供应商安全违约记分抵扣标准.*?(?:\d+\.\d+)?)", std::regex::ECMAScript),
    std::regex(R"(炼铁厂生产协力供应商绩效评价条款.*?(?:\d+\.\d+)?)", std::regex::ECMAScript),
};

// 非考核行过滤模式
static std::vector<std::regex> NON_ASSESSMENT_PATTERNS = {
    std::regex(R"(自查隐患)", std::regex::ECMAScript),
    std::regex(R"(整改闭环)", std::regex::ECMAScript),
    std::regex(R"(嘉奖金额)", std::regex::ECMAScript),
    std::regex(R"(作业费用)", std::regex::ECMAScript),
    std::regex(R"(合同考核)", std::regex::ECMAScript),
    std::regex(R"(考核金额合计)", std::regex::ECMAScript),
    std::regex(R"(当月结算费用)", std::regex::ECMAScript),
    std::regex(R"(小计)", std::regex::ECMAScript),
    std::regex(R"(乙方考核)", std::regex::ECMAScript),
    std::regex(R"(安全考核)", std::regex::ECMAScript),
};

// ============================================================
// 辅助函数
// ============================================================

static std::string _extract_first(const std::string& text, const std::regex& pattern, int group = 1) {
    std::smatch m;
    if (std::regex_search(text, m, pattern) && m.size() > static_cast<size_t>(group)) {
        return m[group].str();
    }
    return "";
}

static double _extract_final_number(const std::string& text) {
    std::regex num_re(R"((?<![-.\d])(\d+(?:,\d+)*(?:\.\d+)?))", std::regex::ECMAScript);
    std::sregex_iterator it(text.begin(), text.end(), num_re);
    std::sregex_iterator end;
    std::string last_match;
    for (; it != end; ++it) {
        last_match = (*it)[1].str();
    }
    if (last_match.empty()) return 0.0;
    // 去除逗号
    last_match.erase(std::remove(last_match.begin(), last_match.end(), ','), last_match.end());
    try {
        return std::stod(last_match);
    } catch (...) {
        return 0.0;
    }
}

static std::string remove_commas(const std::string& s) {
    std::string out = s;
    out.erase(std::remove(out.begin(), out.end(), ','), out.end());
    return out;
}

// ============================================================
// PDF 文本提取
// ============================================================

static std::string extract_pdf_text(const std::string& pdf_path) {
    // 尝试使用 poppler-cpp 的 pdftotext 命令行工具（便携版或系统安装版）
    // 这是跨平台兼容的方式，避免直接链接 poppler-cpp 库的复杂性
    
    fs::path pdf_fs(pdf_path);
    if (!fs::exists(pdf_fs)) {
        throw ParseError("PDF文件不存在: " + pdf_path);
    }

    // 查找 pdftotext 工具
#ifdef _WIN32
    std::string pdftotext_cmd = "pdftotext.exe";
#else
    std::string pdftotext_cmd = "pdftotext";
#endif

    // 创建临时输出文件
    fs::path temp_dir = fs::temp_directory_path();
    fs::path temp_out = temp_dir / "xingda_pdf_text_temp.txt";

    // 构建命令
    std::string cmd = pdftotext_cmd + " -layout \"" + pdf_path + "\" \"" + temp_out.string() + "\" 2>&1";
    
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    
    if (pipe) {
#ifdef _WIN32
        _pclose(pipe);
#else
        pclose(pipe);
#endif
    }

    // 读取临时输出文件
    std::string result;
    if (fs::exists(temp_out)) {
        std::ifstream in(temp_out, std::ios::binary);
        std::stringstream buf;
        buf << in.rdbuf();
        result = buf.str();
        fs::remove(temp_out);
    }

    return result;
}

// ============================================================
// OCR 回退（使用 Tesseract 命令行工具）
// ============================================================

static std::string ocr_pdf(const std::string& pdf_path, int dpi = 300) {
    // 首先尝试用 pdftoppm 将 PDF 转为图片
    // 然后用 tesseract 进行 OCR
    // 这是一个简化的实现
    
    fs::path temp_dir = fs::temp_directory_path();
    fs::path image_prefix = temp_dir / "xingda_ocr_page";

#ifdef _WIN32
    std::string pdftoppm_cmd = "pdftoppm.exe";
    std::string tesseract_cmd = "tesseract.exe";
#else
    std::string pdftoppm_cmd = "pdftoppm";
    std::string tesseract_cmd = "tesseract";
#endif

    // pdftoppm 转换 PDF 页面为图片
    std::string conv_cmd = pdftoppm_cmd + " -r " + std::to_string(dpi)
        + " \"" + pdf_path + "\" \"" + image_prefix.string() + "\" -png 2>&1";

#ifdef _WIN32
    FILE* conv_pipe = _popen(conv_cmd.c_str(), "r");
    if (conv_pipe) _pclose(conv_pipe);
#else
    FILE* conv_pipe = popen(conv_cmd.c_str(), "r");
    if (conv_pipe) pclose(conv_pipe);
#endif

    // 查找生成的图片文件并逐一 OCR
    std::string full_text;
    int page_count = 0;

    // 扫描 temp 目录找到 xingda_ocr_page-*.png
    for (const auto& entry : fs::directory_iterator(temp_dir)) {
        std::string fname = entry.path().filename().string();
        if (fname.find("xingda_ocr_page-") == 0 && fname.find(".png") != std::string::npos) {
            page_count++;
            fs::path ocr_out = temp_dir / ("xingda_ocr_result_" + std::to_string(page_count));

            std::string ocr_cmd = tesseract_cmd + " \"" + entry.path().string()
                + "\" \"" + ocr_out.string() + "\" -l chi_sim+eng 2>&1";

#ifdef _WIN32
            FILE* ocr_pipe = _popen(ocr_cmd.c_str(), "r");
            if (ocr_pipe) _pclose(ocr_pipe);
#else
            FILE* ocr_pipe = popen(ocr_cmd.c_str(), "r");
            if (ocr_pipe) pclose(ocr_pipe);
#endif

            // 读取 OCR 结果
            fs::path ocr_txt = temp_dir / ("xingda_ocr_result_" + std::to_string(page_count) + ".txt");
            if (fs::exists(ocr_txt)) {
                std::ifstream in(ocr_txt);
                std::stringstream buf;
                buf << in.rdbuf();
                full_text += "=== 第 " + std::to_string(page_count) + " 页 ===\n" + buf.str() + "\n";
                fs::remove(ocr_txt);
            }
            fs::remove(entry.path());
        }
    }

    return full_text;
}

// ============================================================
// 提取合同基本信息
// ============================================================

static void extract_contract_info(SettlementData& data, const std::string& full_text) {
    data.contract_no = _extract_first(full_text,
        std::regex(R"(合同编号[：:]\s*((?:SC|HT|W)-\S+))", std::regex::ECMAScript));
    data.work_period = _extract_first(full_text,
        std::regex(R"(作业时间[：:]\s*([^\n]+))", std::regex::ECMAScript));
    data.contract_name = _extract_first(full_text,
        std::regex(R"(合同名称[：:]\s*([^\n]+))", std::regex::ECMAScript));

    // 提取月份标签
    std::smatch month_match;
    if (std::regex_search(data.work_period, month_match,
            std::regex(R"((\d{4})\s*年\s*(\d{1,2})\s*月)", std::regex::ECMAScript))) {
        data.month_label = month_match[2].str() + "\u6708"; // + "月"
    } else {
        // 从文件名提取
        fs::path pdfp(data.pdf_path);
        std::string fname = pdfp.stem().string();
        std::smatch ym_match;
        if (std::regex_search(fname, ym_match,
                std::regex(R"((\d{4})(\d{2}))", std::regex::ECMAScript))) {
            data.month_label = std::to_string(std::stoi(ym_match[2].str())) + "\u6708";
        }
    }
}

// ============================================================
// 提取嘉奖金额（多策略匹配）
// ============================================================

static std::optional<double> extract_reward_amount(const std::string& full_text) {
    std::string num_re = R"((\d+(?:,\d+)*(?:\.\d+)?))";
    std::string opt_space = R"(\s*\n?\s*)";

    // 策略1: 嘉奖金额 \n A \n 小计 \n B
    std::string pat1 = std::string("嘉奖金额") + opt_space + num_re + opt_space + "小计" + opt_space + num_re;
    std::regex re1(pat1, std::regex::ECMAScript);
    std::smatch m1;
    if (std::regex_search(full_text, m1, re1)) {
        try {
            return std::stod(remove_commas(m1[2].str()));
        } catch (...) {}
    }

    // 策略2: 嘉奖金额后直接跟金额
    std::string pat2 = std::string("嘉奖金额[：:]*") + opt_space + num_re;
    std::regex re2(pat2, std::regex::ECMAScript);
    std::smatch m2;
    if (std::regex_search(full_text, m2, re2)) {
        try {
            return std::stod(remove_commas(m2[1].str()));
        } catch (...) {}
    }

    // 策略3: 找到"嘉奖金额"行号，向下扫描
    std::regex num_re_c(R"(\d+(?:,\d+)*(?:\.\d+)?)", std::regex::ECMAScript);
    std::istringstream stream(full_text);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    for (size_t idx = 0; idx < lines.size(); ++idx) {
        if (lines[idx].find("\u5609\u5956\u91d1\u989d") != std::string::npos) { // "嘉奖金额"
            for (int offset = 0; offset <= REWARD_SCAN_LINES; ++offset) {
                if (idx + offset >= lines.size()) break;
                std::string scan_line = lines[idx + offset];
                if (scan_line.find("\u5609\u5956\u91d1\u989d") == 0 && offset == 0) {
                    std::smatch all_m;
                    std::string temp = scan_line;
                    double last_val = 0.0;
                    while (std::regex_search(temp, all_m, num_re_c)) {
                        try { last_val = std::stod(remove_commas(all_m[0].str())); } catch (...) {}
                        temp = all_m.suffix();
                    }
                    if (last_val > REWARD_FILTER_THRESHOLD) return last_val;
                    continue;
                }
                std::smatch m_line;
                std::string line_pat = std::string(R"(^\s*)") + num_re + R"(\s*$)";
                std::regex line_re(line_pat, std::regex::ECMAScript);
                if (std::regex_match(scan_line, m_line, line_re)) {
                    try {
                        double val = std::stod(remove_commas(m_line[1].str()));
                        return val;
                    } catch (...) {}
                }
            }
            break;
        }
    }

    // 策略4: 在"嘉奖金额"与"考核金额合计"之间找最大值
    std::regex section_re(R"(嘉奖金额(.*?)(?:考核金额合计|合同考核))", std::regex::ECMAScript);
    std::smatch sec_m;
    if (std::regex_search(full_text, sec_m, section_re)) {
        std::string section = sec_m[1].str();
        std::sregex_iterator it(section.begin(), section.end(), num_re_c);
        std::sregex_iterator end;
        double max_val = 0.0;
        for (; it != end; ++it) {
            try {
                double v = std::stod(remove_commas((*it)[0].str()));
                if (v > REWARD_FILTER_THRESHOLD && v > max_val) max_val = v;
            } catch (...) {}
        }
        if (max_val > 0.0) return max_val;
    }

    return std::nullopt;
}

// ============================================================
// 提取费用信息
// ============================================================

static void extract_fee_info(SettlementData& data, const std::string& full_text) {
    // 策略1: 从底部合计行提取
    std::regex sum_re(
        R"(合计\s+(\d[\d,.]*)\s+(\d[\d,.]*)\s+(\d[\d,.]*)\s+(\d[\d,.]*))",
        std::regex::ECMAScript
    );
    std::smatch sum_m;
    if (std::regex_search(full_text, sum_m, sum_re)) {
        data.work_fee = std::stod(remove_commas(sum_m[1].str()));
        data.pdf_stated_total = std::stod(remove_commas(sum_m[2].str()));
        data.total_reward = std::stod(remove_commas(sum_m[3].str()));
        data.settlement_amount = std::stod(remove_commas(sum_m[4].str()));
        return;
    }

    // 策略2: 单独匹配作业费用小计
    std::regex work_sub_re(
        R"(作业费用\s*\n(?:.*\n)*?小计\s*(\d+(?:,\d+)*(?:\.\d+)?))",
        std::regex::ECMAScript
    );
    std::smatch wm;
    if (std::regex_search(full_text, wm, work_sub_re)) {
        data.work_fee = std::stod(remove_commas(wm[1].str()));
    }

    // 嘉奖金额
    auto reward = extract_reward_amount(full_text);
    if (reward.has_value()) data.total_reward = reward.value();

    // 策略3: PDF 底部考核金额合计
    std::regex total_re(R"((?:考核金额合计|合同考核.*?小计|总[计和])\s*\n?\s*(\d+(?:,\d+)*(?:\.\d+)?))",
        std::regex::ECMAScript);
    std::smatch tm;
    if (std::regex_search(full_text, tm, total_re)) {
        data.pdf_stated_total = std::stod(remove_commas(tm[1].str()));
    } else {
        // 推断：找最大金额 > 10000
        std::regex amt_re(R"(\d{1,3}(?:,\d{3})*(?:\.\d+)?)", std::regex::ECMAScript);
        std::sregex_iterator it(full_text.begin(), full_text.end(), amt_re);
        std::sregex_iterator end;
        double max_amt = 0.0;
        for (; it != end; ++it) {
            try {
                double v = std::stod(remove_commas((*it)[0].str()));
                if (v > 10000 && v > max_amt) max_amt = v;
            } catch (...) {}
        }
        if (max_amt > 0.0) data.pdf_stated_total = max_amt;
    }

    // 当月结算费用
    if (data.settlement_amount == 0.0) {
        std::regex settle_re(R"(当月结算费用[：:]*\s*(\d+(?:,\d+)*(?:\.\d+)?))", std::regex::ECMAScript);
        std::smatch sm;
        if (std::regex_search(full_text, sm, settle_re)) {
            data.settlement_amount = std::stod(remove_commas(sm[1].str()));
        }
    }
}

// ============================================================
// 通道2: 文本行提取（主通道）
// ============================================================

// ============================================================
// 从文本行构建 AssessmentRecord
// ============================================================

static std::optional<AssessmentRecord> build_text_record(const std::string& full_line) {
    std::smatch idx_match;
    if (!std::regex_search(full_line, idx_match, std::regex(R"(^(\d{1,2})\s+)"))) {
        return std::nullopt;
    }

    int index = std::stoi(idx_match[1].str());
    if (index < 1 || index > MAX_ITEM_INDEX) return std::nullopt;

    // 过滤非考核行
    for (const auto& pat : NON_ASSESSMENT_PATTERNS) {
        if (std::regex_search(full_line, pat)) return std::nullopt;
    }

    std::string remainder = idx_match.suffix();

    // 提取条款
    std::vector<std::string> clauses;
    for (const auto& pat : CLAUSE_PATTERNS) {
        std::sregex_iterator it(remainder.begin(), remainder.end(), pat);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            clauses.push_back((*it)[0].str());
        }
    }

    // 提取金额
    double amount = _extract_final_number(remainder);
    if (amount < MIN_ASSESSMENT_AMOUNT) return std::nullopt;

    // 清理描述
    std::string desc = remainder;
    for (const auto& clause : clauses) {
        auto pos = desc.find(clause);
        if (pos != std::string::npos) {
            desc.erase(pos, clause.length());
        }
    }

    // 移除末尾金额
    std::regex trailing_num(R"(\d+(?:,\d+)*(?:\.\d+)??\s*$)");
    desc = std::regex_replace(desc, trailing_num, "");
    // 合并多余空白
    desc = std::regex_replace(desc, std::regex(R"(\s+)"), "");

    // 校验描述开头
    std::regex desc_prefix_re(std::string("^(") + ASSESSMENT_DESC_PREFIX_PAT + ")",
        std::regex::ECMAScript);
    if (!std::regex_search(desc, desc_prefix_re)) return std::nullopt;

    std::string clause_str;
    for (size_t i = 0; i < clauses.size(); ++i) {
        if (i > 0) clause_str += "\uff1b"; // "；"
        clause_str += clauses[i];
    }

    AssessmentRecord rec;
    rec.index = index;
    rec.description = desc;
    rec.clause = clause_str;
    rec.amount = amount;
    rec.parse_source = "text";
    return rec;
}

// ============================================================
// 通道2: 文本行提取（主通道）
// ============================================================

static std::vector<AssessmentRecord> extract_from_text(const std::string& full_text) {
    std::vector<AssessmentRecord> records;
    std::istringstream stream(full_text);
    std::string line;
    std::vector<std::string> lines;

    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    // 章节边界标记
    std::vector<std::regex> boundary_patterns = {
        std::regex(R"(^小计\s*\d+)", std::regex::ECMAScript),
        std::regex(R"(^合同嘉奖)", std::regex::ECMAScript),
        std::regex(R"(^嘉奖金额)", std::regex::ECMAScript),
    };

    std::vector<std::string> current_record_lines;
    bool stopped = false;

    for (const auto& raw_line : lines) {
        if (stopped) break;

        std::string ln = raw_line;
        auto start = ln.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        ln = ln.substr(start);

        for (const auto& bp : boundary_patterns) {
            if (std::regex_search(ln, bp)) {
                if (!current_record_lines.empty()) {
                    std::string full_line;
                    for (size_t i = 0; i < current_record_lines.size(); ++i) {
                        if (i > 0) full_line += " ";
                        full_line += current_record_lines[i];
                    }
                    auto rec = build_text_record(full_line);
                    if (rec.has_value()) {
                        rec->parse_source = "text";
                        records.push_back(rec.value());
                    }
                }
                current_record_lines.clear();
                stopped = true;
                break;
            }
        }
        if (stopped) break;

        std::smatch m;
        if (std::regex_search(ln, m, RECORD_LINE_RE)) {
            if (!current_record_lines.empty()) {
                std::string full_line;
                for (size_t i = 0; i < current_record_lines.size(); ++i) {
                    if (i > 0) full_line += " ";
                    full_line += current_record_lines[i];
                }
                auto rec = build_text_record(full_line);
                if (rec.has_value()) {
                    rec->parse_source = "text";
                    records.push_back(rec.value());
                }
            }
            current_record_lines = {ln};
        } else if (!current_record_lines.empty()) {
            static std::regex skip_re(R"(^(第\d+页|编号|项目|合计|小计|总计|合同嘉奖|嘉奖))", std::regex::ECMAScript);
            if (!std::regex_search(ln, skip_re)) {
                current_record_lines.push_back(ln);
            }
        }
    }

    if (!stopped && !current_record_lines.empty()) {
        std::string full_line;
        for (size_t i = 0; i < current_record_lines.size(); ++i) {
            if (i > 0) full_line += " ";
            full_line += current_record_lines[i];
        }
        auto rec = build_text_record(full_line);
        if (rec.has_value()) {
            rec->parse_source = "text";
            records.push_back(rec.value());
        }
    }

    return records;
}

// ============================================================
// 主解析函数
// ============================================================

SettlementData parse_pdf(const std::string& pdf_path, const ParserConfig& pconfig) {
    fs::path pdf_fs(pdf_path);
    if (!fs::exists(pdf_fs)) {
        throw ParseError("PDF文件不存在: " + pdf_path);
    }

    SettlementData data;
    data.pdf_path = pdf_path;

    // 提取文本
    std::string full_text;
    try {
        full_text = extract_pdf_text(pdf_path);
    } catch (const std::exception& e) {
        throw ParseError(std::string("无法读取PDF文件: ") + e.what());
    }

    data.raw_text = full_text;

    // OCR 回退
    if (full_text.empty()) {
        try {
            full_text = ocr_pdf(pdf_path, pconfig.ocr_dpi);
            data.from_ocr = true;
        } catch (...) {
            throw ParseError("PDF文本提取和OCR均失败");
        }
        if (full_text.empty()) {
            throw ParseError("PDF无法提取任何文本内容");
        }
    }

    // 提取合同基本信息
    extract_contract_info(data, full_text);

    // 提取费用信息
    extract_fee_info(data, full_text);

    // 通道2: 文本行解析
    auto text_records = extract_from_text(full_text);

    data.all_records = std::move(text_records);

    return data;
}

} // namespace xingda