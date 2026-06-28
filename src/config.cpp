#include "config.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <regex>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif

namespace xingda {

// ============================================================
// 简单的 YAML 解析器（仅支持 classify_rules.yaml 的格式）
// ============================================================

namespace {
    // 去除首尾空白
    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    // 去除注释（# 之后的部分）
    std::string strip_comment(const std::string& line) {
        auto pos = line.find('#');
        if (pos == 0) return "";
        if (pos != std::string::npos) {
            // 检查 # 是否在引号内（简单处理）
            return line.substr(0, pos);
        }
        return line;
    }

    // 提取 YAML 键值对中的字符串值（引号内）
    std::string extract_quoted_string(const std::string& line) {
        auto start = line.find('"');
        if (start != std::string::npos) {
            auto end = line.find('"', start + 1);
            if (end != std::string::npos) {
                return line.substr(start + 1, end - start - 1);
            }
        }
        // 无引号，取冒号后的内容
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            return trim(line.substr(colon + 1));
        }
        return "";
    }

    // 提取冒号后不带引号的字符串值
    std::string extract_value(const std::string& line) {
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string val = line.substr(colon + 1);
            val = strip_comment(val);
            val = trim(val);
            // 去除两端引号
            if (!val.empty() && val.front() == '"' && val.back() == '"') {
                val = val.substr(1, val.size() - 2);
            }
            return val;
        }
        return "";
    }

    // 提取列表项（以 - 开头）
    std::string extract_list_item(const std::string& line) {
        auto dash = line.find('-');
        if (dash == std::string::npos) return "";
        std::string item = line.substr(dash + 1);
        // 去除引号
        item = trim(item);
        if (!item.empty() && item.front() == '"' && item.back() == '"') {
            item = item.substr(1, item.size() - 2);
        }
        return item;
    }

    // 提取缩进层级
    int get_indent(const std::string& line) {
        int indent = 0;
        for (char c : line) {
            if (c == ' ') indent++;
            else break;
        }
        return indent;
    }
} // anonymous namespace

ClassifyRules ClassifyRules::load_default() {
    ClassifyRules rules;
    rules.department_ratio = 0.01;
    rules.area_order = {
        "\u4e8b\u4e1a\u90e8",       // 事业部
        "\u539f\u6599\u5206\u5382\u4f5c\u4e1a\u533a", // 原料分厂作业区
        "\u4f9b\u77ff\u4f5c\u4e1a\u533a",             // 供矿作业区
        "\u7164\u5e93\u4f5c\u4e1a\u533a",             // 煤库作业区
        "\u672a\u5206\u7c7b"                          // 未分类
    };

    // 事业部
    AreaRule ar1;
    ar1.name = "\u4e8b\u4e1a\u90e8";
    ar1.priority = 1;
    ar1.description_patterns = {
        "\u534f\u529b\u5b89\u5168\u7ba1\u7406\u5de5\u4f5c\u65b9\u6848.*\u843d\u5b9e",
        "\u5408\u540c\u8bc4\u4ef7.*\u6392\u540d"
    };
    ar1.has_match_rules = true;
    ar1.compile_regexes();
    rules.areas.push_back(ar1);

    // 供矿作业区
    AreaRule ar2;
    ar2.name = "\u4f9b\u77ff\u4f5c\u4e1a\u533a";
    ar2.priority = 2;
    ar2.keywords = {"\u4f9b\u77ff", "\u7ffb\u8f66", "\u7403\u56e2"};
    ar2.has_match_rules = true;
    ar2.compile_regexes();
    rules.areas.push_back(ar2);

    // 煤库作业区
    AreaRule ar3;
    ar3.name = "\u7164\u5e93\u4f5c\u4e1a\u533a";
    ar3.priority = 3;
    ar3.keywords = {"\u7164\u5e93", "\u539f\u7164\u4ed3", "\u539f\u7164", "\u5378\u7164\u95f4"};
    ar3.equipment_prefixes = {"M"};
    ar3.has_match_rules = true;
    ar3.compile_regexes();
    rules.areas.push_back(ar3);

    // 原料分厂作业区
    AreaRule ar4;
    ar4.name = "\u539f\u6599\u5206\u5382\u4f5c\u4e1a\u533a";
    ar4.priority = 4;
    ar4.keywords = {"\u539f\u6599\u5206\u5382", "\u8f93\u5165\u4f5c\u4e1a\u533a",
                    "\u8f93\u5165\u533a\u57df", "\u539f\u6599\u533a\u57df",
                    "\u539f\u6599\u8f93\u5165", "\u539f\u6599\u73ed",
                    "\u534f\u529b\u7cfb\u7edf", "\u5174\u8fbe\u539f\u6599\u4f5c\u4e1a\u533a"};
    ar4.equipment_prefixes = {"B", "E", "F", "K", "N", "C"};
    ar4.has_match_rules = true;
    ar4.compile_regexes();
    rules.areas.push_back(ar4);

    // 未分类（兜底）
    AreaRule ar5;
    ar5.name = "\u672a\u5206\u7c7b";
    ar5.priority = 99;
    ar5.has_match_rules = false;
    ar5.compile_regexes();
    rules.areas.push_back(ar5);

    return rules;
}

ClassifyRules ClassifyRules::load_from_yaml(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::fprintf(stderr, "[警告] 找不到配置文件 '%s'，使用内置默认规则。\n", filepath.c_str());
        return load_default();
    }

    ClassifyRules rules;
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    enum class Section { None, Areas, AreaItem, Match };
    Section section = Section::None;
    AreaRule current_area;

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string raw = strip_comment(lines[i]);
        std::string trimmed = trim(raw);
        if (trimmed.empty()) continue;

        int indent = get_indent(raw);

        // 顶层键
        if (indent == 0) {
            if (trimmed.find("areas:") == 0) {
                section = Section::Areas;
            } else if (trimmed.find("department_ratio:") == 0) {
                std::string val = extract_value(trimmed);
                try { rules.department_ratio = std::stod(val); } catch (...) {}
                section = Section::None;
            } else if (trimmed.find("area_order:") == 0) {
                section = Section::None;
                // 读取 area_order 列表项
                for (size_t j = i + 1; j < lines.size(); ++j) {
                    std::string r2 = strip_comment(lines[j]);
                    std::string t2 = trim(r2);
                    if (t2.empty()) continue;
                    int i2 = get_indent(r2);
                    if (i2 == 0) break;
                    std::string item = extract_list_item(t2);
                    if (!item.empty()) {
                        rules.area_order.push_back(item);
                    }
                }
            } else {
                section = Section::None;
            }
            continue;
        }

        // areas 子项
        if (section == Section::Areas || section == Section::AreaItem || section == Section::Match) {
            if (indent == 2 && trimmed.find("- name:") == 0) {
                // 保存上一个区域
                if (!current_area.name.empty()) {
                    current_area.compile_regexes();
                    rules.areas.push_back(current_area);
                    current_area = AreaRule{};
                }
                current_area.name = extract_quoted_string(trimmed);
                section = Section::AreaItem;
            } else if (indent == 4) {
                if (trimmed.find("priority:") == 0) {
                    std::string val = extract_value(trimmed);
                    try { current_area.priority = std::stoi(val); } catch (...) {}
                } else if (trimmed.find("match:") == 0) {
                    section = Section::Match;
                    // 检查 match: {} 空对象
                    if (trimmed.find("{}") != std::string::npos) {
                        current_area.has_match_rules = false;
                    } else {
                        current_area.has_match_rules = true;
                    }
                }
            } else if (indent == 6 || indent == 8) {
                if (section == Section::Match) {
                    if (trimmed.find("item_numbers:") == 0) {
                        // 读取列表项
                        for (size_t j = i + 1; j < lines.size(); ++j) {
                            std::string r3 = strip_comment(lines[j]);
                            std::string t3 = trim(r3);
                            if (t3.empty()) continue;
                            int i3 = get_indent(r3);
                            if (i3 <= 6) break;
                            std::string item = extract_list_item(t3);
                            if (!item.empty()) {
                                try { current_area.item_numbers.push_back(std::stoi(item)); } catch (...) {}
                            }
                        }
                    } else if (trimmed.find("keywords:") == 0) {
                        // 读取 keywords 列表
                        for (size_t j = i + 1; j < lines.size(); ++j) {
                            std::string r3 = strip_comment(lines[j]);
                            std::string t3 = trim(r3);
                            if (t3.empty()) continue;
                            int i3 = get_indent(r3);
                            if (i3 <= 6) break;
                            std::string item = extract_list_item(t3);
                            if (!item.empty()) {
                                current_area.keywords.push_back(item);
                            }
                        }
                    } else if (trimmed.find("equipment_prefixes:") == 0) {
                        for (size_t j = i + 1; j < lines.size(); ++j) {
                            std::string r3 = strip_comment(lines[j]);
                            std::string t3 = trim(r3);
                            if (t3.empty()) continue;
                            int i3 = get_indent(r3);
                            if (i3 <= 6) break;
                            std::string item = extract_list_item(t3);
                            if (!item.empty()) {
                                current_area.equipment_prefixes.push_back(item);
                            }
                        }
                    } else if (trimmed.find("description_patterns:") == 0) {
                        for (size_t j = i + 1; j < lines.size(); ++j) {
                            std::string r3 = strip_comment(lines[j]);
                            std::string t3 = trim(r3);
                            if (t3.empty()) continue;
                            int i3 = get_indent(r3);
                            if (i3 <= 6) break;
                            std::string item = extract_list_item(t3);
                            if (!item.empty()) {
                                current_area.description_patterns.push_back(item);
                            }
                        }
                    }
                }
            }
        }
    }

    // 保存最后一个区域
    if (!current_area.name.empty()) {
        current_area.compile_regexes();
        rules.areas.push_back(current_area);
    }

    // 确保有未分类兜底
    bool has_fallback = false;
    for (const auto& a : rules.areas) {
        if (a.name == "\u672a\u5206\u7c7b") { has_fallback = true; break; }
    }
    if (!has_fallback) {
        AreaRule fb;
        fb.name = "\u672a\u5206\u7c7b";
        fb.priority = 99;
        fb.has_match_rules = false;
        fb.compile_regexes();
        rules.areas.push_back(fb);
    }

    // 按 priority 排序
    std::sort(rules.areas.begin(), rules.areas.end(),
        [](const AreaRule& a, const AreaRule& b) { return a.priority < b.priority; });

    // 如果 area_order 为空，从 areas 生成
    if (rules.area_order.empty()) {
        for (const auto& a : rules.areas) {
            rules.area_order.push_back(a.name);
        }
    }

    return rules;
}

std::string find_default_rules_path() {
    namespace fs = std::filesystem;

    // 1. EXE 同目录
    try {
#ifdef _WIN32
        char buf[MAX_PATH];
        GetModuleFileNameA(nullptr, buf, MAX_PATH);
        fs::path exe_dir = fs::path(buf).parent_path();
#else
        fs::path exe_dir = fs::current_path();
#endif
        auto p = exe_dir / "classify_rules.yaml";
        if (fs::exists(p)) return p.string();
    } catch (...) {}

    // 2. 当前工作目录
    try {
        auto p = fs::current_path() / "classify_rules.yaml";
        if (fs::exists(p)) return p.string();
    } catch (...) {}

    return "";
}

} // namespace xingda