#include "cli.h"
#include "parser.h"
#include "classifier.h"
#include "validator.h"
#include "excel_writer.h"
#include "config.h"
#include "exceptions.h"
#include <fstream>
#include <filesystem>

namespace xingda {

namespace fs = std::filesystem;

std::string process_pdf_core(
    const std::string& pdf_path,
    const std::string& output_dir,
    const std::optional<std::string>& rules_path,
    bool validate_only,
    bool dump_text,
    bool include_summary,
    bool /*enable_ocr*/,
    bool summary_only,
    bool /*no_merge*/,
    const ParserConfig& parser_config,
    const std::optional<std::string>& style_name)
{
    if (!fs::exists(pdf_path)) {
        throw ParseError("PDF文件不存在: " + pdf_path);
    }

    // 加载配置
    ClassifyRules rules;
    if (rules_path.has_value() && !rules_path->empty()) {
        rules = ClassifyRules::load_from_yaml(rules_path.value());
    } else {
        std::string default_path = find_default_rules_path();
        if (!default_path.empty()) {
            rules = ClassifyRules::load_from_yaml(default_path);
        } else {
            rules = ClassifyRules::load_default();
        }
    }

    ExcelStyle excel_style = ExcelStyle::default_style();
    if (style_name.has_value()) {
        if (style_name.value() == "compact") {
            excel_style = ExcelStyle::compact();
        } else if (style_name.value() == "wide") {
            excel_style = ExcelStyle::wide();
        }
    }

    // --- 1. 解析 PDF ---
    SettlementData data = parse_pdf(pdf_path, parser_config);

    // 导出原始文本
    if (dump_text) {
        fs::path pdf_fs(pdf_path);
        fs::path txt_path = pdf_fs;
        txt_path.replace_extension(".txt");
        std::string header = "=== PDF 原始文本导出 ===\n文件: " + pdf_path
            + "\n提取字符数: " + std::to_string(data.raw_text.size()) + "\n=========================\n\n";
        std::ofstream txt_file(txt_path, std::ios::binary);
        txt_file << header << data.raw_text;
        txt_file.close();
    }

    // --- 2. 分类 ---
    classify_records(data, rules);

    // --- 3. 校验 ---
    bool is_valid = validate_amounts(data);

    if (validate_only) {
        return is_valid ? "" : "";
    }

    // --- 4. 生成 Excel ---
    fs::path pdf_fs(pdf_path);
    std::string excel_name = pdf_fs.stem().string() + "\u660e\u7ec6.xls"; // "明细.xls"
    fs::path out_dir(output_dir);
    fs::create_directories(out_dir);
    fs::path output_path = out_dir / excel_name;

    generate_excel(
        data,
        output_path.string(),
        rules.area_order,
        excel_style,
        include_summary,
        summary_only
    );

    return output_path.string();
}

} // namespace xingda