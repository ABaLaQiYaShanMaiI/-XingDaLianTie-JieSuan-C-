#include "excel_writer.h"
#include "exceptions.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <ctime>
#include <algorithm>

namespace xingda {

namespace fs = std::filesystem;

// ============================================================
// 使用 Excel XML Spreadsheet 2003 格式生成 .xls 文件
// 完全自包含，无需任何第三方 Excel 库
// ============================================================

static std::string escape_xml(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '&': out += "&"; break;
            case '<': out += "<"; break;
            case '>': out += ">"; break;
            case '"': out += """; break;
            case '\'': out += "'"; break;
            case '\n': out += "&#10;"; break;
            default: out += c;
        }
    }
    return out;
}

static std::string fmt_xml_number(double v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << v;
    return ss.str();
}

static std::string fmt_xml_number_int(int v) {
    return std::to_string(v);
}

void generate_excel(
    const SettlementData& data,
    const std::string& output_path,
    const std::vector<std::string>& area_order,
    const ExcelStyle& style,
    bool include_summary,
    bool summary_only)
{
    fs::path out_fs(output_path);
    if (out_fs.has_parent_path()) {
        fs::create_directories(out_fs.parent_path());
    }

    std::ofstream file(output_path, std::ios::binary);
    if (!file.is_open()) {
        throw ExcelWriteError("无法创建输出文件: " + output_path);
    }

    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<?mso-application progid=\"Excel.Sheet\"?>\n";
    file << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
    file << " xmlns:o=\"urn:schemas-microsoft-com:office:office\"\n";
    file << " xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\n";
    file << " xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
    file << " xmlns:html=\"http://www.w3.org/TR/REC-html40\">\n";

    // 样式定义
    file << "<Styles>\n";

    // Default style (s0)
    file << " <Style ss:ID=\"Default\" ss:Name=\"Normal\">\n";
    file << "  <Font ss:FontName=\"" << escape_xml(style.font_name)
         << "\" ss:Size=\"" << static_cast<int>(style.font_size) << "\"/>\n";
    file << " </Style>\n";

    // Bold centered (s1)
    file << " <Style ss:ID=\"s1\">\n";
    file << "  <Font ss:FontName=\"" << escape_xml(style.font_name)
         << "\" ss:Size=\"" << static_cast<int>(style.font_size) << "\" ss:Bold=\"1\"/>\n";
    file << "  <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    file << "  <Borders><Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/></Borders>\n";
    file << " </Style>\n";

    // Left aligned with border (s2)
    file << " <Style ss:ID=\"s2\">\n";
    file << "  <Font ss:FontName=\"" << escape_xml(style.font_name)
         << "\" ss:Size=\"" << static_cast<int>(style.font_size) << "\"/>\n";
    file << "  <Alignment ss:Horizontal=\"Left\" ss:Vertical=\"Center\" ss:WrapText=\"1\"/>\n";
    file << "  <Borders><Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/></Borders>\n";
    file << " </Style>\n";

    // Center with border and wrap (s3)
    file << " <Style ss:ID=\"s3\">\n";
    file << "  <Font ss:FontName=\"" << escape_xml(style.font_name)
         << "\" ss:Size=\"" << static_cast<int>(style.font_size) << "\" ss:Bold=\"1\"/>\n";
    file << "  <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\" ss:WrapText=\"1\"/>\n";
    file << "  <Borders><Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/></Borders>\n";
    file << " </Style>\n";

    // Number with border (s4)
    file << " <Style ss:ID=\"s4\">\n";
    file << "  <Font ss:FontName=\"" << escape_xml(style.font_name)
         << "\" ss:Size=\"" << static_cast<int>(style.font_size) << "\"/>\n";
    file << "  <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    file << "  <NumberFormat ss:Format=\"#,##0.00\"/>\n";
    file << "  <Borders><Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/></Borders>\n";
    file << " </Style>\n";

    // Red bold warning (s5)
    file << " <Style ss:ID=\"s5\">\n";
    file << "  <Font ss:FontName=\"" << escape_xml(style.font_name)
         << "\" ss:Size=\"" << static_cast<int>(style.font_size) << "\" ss:Bold=\"1\" ss:Color=\"#FF0000\"/>\n";
    file << "  <Alignment ss:Horizontal=\"Left\" ss:Vertical=\"Center\" ss:WrapText=\"1\"/>\n";
    file << "  <Borders><Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/></Borders>\n";
    file << " </Style>\n";

    // Center plain with border (s6)
    file << " <Style ss:ID=\"s6\">\n";
    file << "  <Font ss:FontName=\"" << escape_xml(style.font_name)
         << "\" ss:Size=\"" << static_cast<int>(style.font_size) << "\"/>\n";
    file << "  <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    file << "  <Borders><Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>"
         << "<Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/></Borders>\n";
    file << " </Style>\n";

    file << "</Styles>\n";

    // Worksheet
    std::string sheet_name = data.month_label.empty() ? "\u7ed3\u7b97\u660e\u7ec6" : data.month_label; // "结算明细"
    file << "<Worksheet ss:Name=\"" << escape_xml(sheet_name) << "\">\n";
    file << "<Table>\n";

    // 设置列宽
    file << " <Column ss:Index=\"1\" ss:Width=\"" << style.col_width_a << "\"/>\n";
    file << " <Column ss:Index=\"2\" ss:Width=\"" << style.col_width_b << "\"/>\n";
    file << " <Column ss:Index=\"3\" ss:Width=\"" << style.col_width_c << "\"/>\n";

    // --- 1. 汇总信息区域 ---
    if (include_summary) {
        // 标题
        file << " <Row>\n";
        file << "  <Cell ss:MergeAcross=\"2\" ss:StyleID=\"s1\"><Data ss:Type=\"String\">"
             << "\u7ed3\u7b97\u5355\u6c47\u603b\u4fe1\u606f</Data></Cell>\n"; // "结算单汇总信息"
        file << " </Row>\n";

        // 合同信息
        auto write_info_row = [&](const std::string& label, const std::string& value, bool is_number = false) {
            file << " <Row>\n";
            file << "  <Cell ss:MergeAcross=\"1\" ss:StyleID=\"s2\"><Data ss:Type=\"String\">"
                 << escape_xml(label) << "</Data></Cell>\n";
            if (is_number) {
                file << "  <Cell ss:StyleID=\"s4\"><Data ss:Type=\"Number\">"
                     << fmt_xml_number(std::stod(value.empty() ? "0" : value))
                     << "</Data></Cell>\n";
            } else {
                file << "  <Cell ss:StyleID=\"s6\"><Data ss:Type=\"String\">"
                     << escape_xml(value) << "</Data></Cell>\n";
            }
            file << " </Row>\n";
        };

        write_info_row("\u5408\u540c\u7f16\u53f7\uff1a", data.contract_no); // "合同编号："
        write_info_row("\u5408\u540c\u540d\u79f0\uff1a", data.contract_name); // "合同名称："
        write_info_row("\u4f5c\u4e1a\u65f6\u95f4\uff1a", data.work_period); // "作业时间："

        // 空行
        file << " <Row>\n </Row>\n";

        write_info_row("\u4f5c\u4e1a\u8d39\u7528", std::to_string(data.work_fee), true); // "作业费用"
        write_info_row("\u8003\u6838\u91d1\u989d\u5408\u8ba1", std::to_string(data.total_assessment), true); // "考核金额合计"
        write_info_row("\u5609\u5956\u91d1\u989d\u5408\u8ba1", std::to_string(data.total_reward), true); // "嘉奖金额合计"
        write_info_row("\u5f53\u6708\u7ed3\u7b97\u8d39\u7528", std::to_string(data.settlement_amount_or_computed()), true); // "当月结算费用"

        // 空行
        file << " <Row>\n </Row>\n";

        // 区域考核概览
        file << " <Row>\n";
        file << "  <Cell ss:MergeAcross=\"2\" ss:StyleID=\"s1\"><Data ss:Type=\"String\">"
             << "\u533a\u57df\u8003\u6838\u6982\u89c8</Data></Cell>\n"; // "区域考核概览"
        file << " </Row>\n";

        // 表头
        file << " <Row>\n";
        file << "  <Cell ss:StyleID=\"s1\"><Data ss:Type=\"String\">"
             << "\u533a\u57df</Data></Cell>\n"; // "区域"
        file << "  <Cell ss:StyleID=\"s1\"><Data ss:Type=\"String\">"
             << "\u6761\u6570</Data></Cell>\n"; // "条数"
        file << "  <Cell ss:StyleID=\"s1\"><Data ss:Type=\"String\">"
             << "\u8003\u6838\u91d1\u989d\u5c0f\u8ba1</Data></Cell>\n"; // "考核金额小计"
        file << " </Row>\n";

        for (const auto& area_name : area_order) {
            auto it = data.areas.find(area_name);
            if (it == data.areas.end() || it->second.records.empty()) continue;
            file << " <Row>\n";
            file << "  <Cell ss:StyleID=\"s6\"><Data ss:Type=\"String\">"
                 << escape_xml(area_name) << "</Data></Cell>\n";
            file << "  <Cell ss:StyleID=\"s6\"><Data ss:Type=\"Number\">"
                 << it->second.records.size() << "</Data></Cell>\n";
            file << "  <Cell ss:StyleID=\"s4\"><Data ss:Type=\"Number\">"
                 << fmt_xml_number(it->second.subtotal) << "</Data></Cell>\n";
            file << " </Row>\n";
        }

        // 空行
        file << " <Row>\n </Row>\n";
    }

    // --- 2. 区域明细 ---
    if (!summary_only) {
        for (size_t idx = 0; idx < area_order.size(); ++idx) {
            const std::string& area_name = area_order[idx];
            auto it = data.areas.find(area_name);
            if (it == data.areas.end() || it->second.records.empty()) continue;

            if (idx > 0) {
                file << " <Row>\n </Row>\n";
            }

            const AreaData& area_data = it->second;

            // 区域标题行
            file << " <Row>\n";
            file << "  <Cell ss:MergeAcross=\"2\" ss:StyleID=\"s1\"><Data ss:Type=\"String\">"
                 << escape_xml(area_data.name) << "</Data></Cell>\n";
            file << " </Row>\n";

            // 表头行
            file << " <Row ss:Height=\"" << style.header_row_height << "\">\n";
            file << "  <Cell ss:StyleID=\"s3\"><Data ss:Type=\"String\">"
                 << "\u8003\u6838\n\u4e8b\u9879</Data></Cell>\n"; // "考核\n事项"
            file << "  <Cell ss:StyleID=\"s3\"><Data ss:Type=\"String\">"
                 << "\u6761\u6b3e</Data></Cell>\n"; // "条款"
            file << "  <Cell ss:StyleID=\"s3\"><Data ss:Type=\"String\">"
                 << "\u8003\u6838\n\u91d1\u989d</Data></Cell>\n"; // "考核\n金额"
            file << " </Row>\n";

            // 数据行
            for (const auto& record : area_data.records) {
                file << " <Row>\n";
                file << "  <Cell ss:StyleID=\"s2\"><Data ss:Type=\"String\">"
                     << escape_xml(record.description) << "</Data></Cell>\n";
                file << "  <Cell ss:StyleID=\"s2\"><Data ss:Type=\"String\">"
                     << escape_xml(record.clause) << "</Data></Cell>\n";
                file << "  <Cell ss:StyleID=\"s4\"><Data ss:Type=\"Number\">"
                     << fmt_xml_number(record.amount) << "</Data></Cell>\n";
                file << " </Row>\n";
            }

            // 小计行
            file << " <Row>\n";
            file << "  <Cell ss:MergeAcross=\"1\" ss:StyleID=\"s1\"><Data ss:Type=\"String\">"
                 << "\u5c0f\u8ba1</Data></Cell>\n"; // "小计"
            file << "  <Cell ss:StyleID=\"s4\"><Data ss:Type=\"Number\">"
                 << fmt_xml_number(area_data.subtotal) << "</Data></Cell>\n";
            file << " </Row>\n";

            // 事业部考核金额行
            file << " <Row>\n";
            file << "  <Cell ss:MergeAcross=\"1\" ss:StyleID=\"s1\"><Data ss:Type=\"String\">"
                 << "\u4e8b\u4e1a\u90e8\u8003\u6838\u91d1\u989d</Data></Cell>\n"; // "事业部考核金额"
            file << "  <Cell ss:StyleID=\"s4\"><Data ss:Type=\"Number\">"
                 << fmt_xml_number(area_data.dept_amount) << "</Data></Cell>\n";
            file << " </Row>\n";
        }
    }

    // --- 3. 校验失败警告行 ---
    if (!data.amount_match) {
        file << " <Row>\n </Row>\n";
        file << " <Row>\n";
        file << "  <Cell ss:MergeAcross=\"2\" ss:StyleID=\"s5\"><Data ss:Type=\"String\">"
             << escape_xml("\u26a0 \u91d1\u989d\u6821\u9a8c\u5931\u8d25\uff1a")
             << escape_xml("PDF \u58f0\u660e\u5408\u8ba1 \u00a5")
             << fmt_xml_number(data.pdf_stated_total.value_or(0.0))
             << escape_xml("\uff0c\u7a0b\u5e8f\u63d0\u53d6\u5408\u8ba1 \u00a5")
             << fmt_xml_number(data.total_assessment)
             << escape_xml("\uff0c\u504f\u5dee ")
             << std::fixed << std::setprecision(2) << (data.amount_deviation_pct * 100.0) << "%"
             << "</Data></Cell>\n";
        file << " </Row>\n";
    }

    file << "</Table>\n";
    file << "</Worksheet>\n";
    file << "</Workbook>\n";

    file.close();
}

} // namespace xingda