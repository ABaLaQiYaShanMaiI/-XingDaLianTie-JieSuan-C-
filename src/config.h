#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <regex>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstring>

namespace xingda {

/// 区域的匹配规则定义
struct AreaRule {
    std::string name;
    int priority = 99;
    std::vector<int> item_numbers;
    std::vector<std::string> keywords;
    std::vector<std::string> equipment_prefixes;
    std::vector<std::string> description_patterns;
    bool has_match_rules = true;

    // 预编译正则
    std::vector<std::regex> compiled_equipment_re;
    std::vector<std::regex> compiled_pattern_re;

    AreaRule() = default;

    void compile_regexes() {
        compiled_equipment_re.clear();
        compiled_pattern_re.clear();
        for (const auto& prefix : equipment_prefixes) {
            std::string pattern = prefix + std::string("\\d+");
            try {
                compiled_equipment_re.emplace_back(pattern);
            } catch (...) {}
        }
        for (const auto& pat : description_patterns) {
            try {
                compiled_pattern_re.emplace_back(pat);
            } catch (...) {}
        }
    }
};

/// YAML 解析的基本结构（不使用第三方库，用手写解析器）
struct ClassifyRules {
    std::vector<AreaRule> areas;
    double department_ratio = 0.01;
    std::vector<std::string> area_order;

    static ClassifyRules load_from_yaml(const std::string& filepath);
    static ClassifyRules load_default();
};

/// Excel 样式配置
struct ExcelStyle {
    std::string font_name = "\u5b8b\u4f53"; // 宋体 (Unicode escapes for cross-platform)
    double font_size = 16;
    double col_width_a = 80.625;
    double col_width_b = 40.625;
    double col_width_c = 22.0;
    double header_row_height = 40.5;

    static ExcelStyle default_style() { return ExcelStyle{}; }

    static ExcelStyle compact() {
        ExcelStyle s;
        s.font_size = 12;
        s.col_width_a = 60.0;
        s.col_width_b = 30.0;
        s.header_row_height = 30.0;
        return s;
    }

    static ExcelStyle wide() {
        ExcelStyle s;
        s.col_width_a = 100.0;
        s.col_width_b = 50.0;
        return s;
    }
};

/// PDF 解析器默认常量配置
struct ParserConfig {
    int max_item_index = 99;
    double min_assessment_amount = 1.0;
    int reward_scan_lines = 5;
    int ocr_dpi = 300;
    double reward_filter_threshold = 10.0;

    static ParserConfig default_config() { return ParserConfig{}; }
};

/// 查找默认规则文件路径
std::string find_default_rules_path();

} // namespace xingda

#endif // CONFIG_H