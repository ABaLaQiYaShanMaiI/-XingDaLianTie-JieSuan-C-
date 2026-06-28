#include "classifier.h"
#include <algorithm>

namespace xingda {

static std::string classify_one(const AssessmentRecord& record, const std::vector<AreaRule>& areas) {
    int idx = record.index;
    const std::string& desc = record.description;

    for (const auto& area_cfg : areas) {
        // 跳过无匹配条件的区域规则（兜底规则）
        if (!area_cfg.has_match_rules) continue;

        // 规则1: 精确序号匹配
        if (std::find(area_cfg.item_numbers.begin(), area_cfg.item_numbers.end(), idx)
            != area_cfg.item_numbers.end()) {
            return area_cfg.name;
        }

        // 规则2: 关键词匹配
        for (const auto& kw : area_cfg.keywords) {
            if (desc.find(kw) != std::string::npos) {
                return area_cfg.name;
            }
        }

        // 规则3: 设备编号前缀正则匹配
        for (const auto& cre : area_cfg.compiled_equipment_re) {
            if (std::regex_search(desc, cre)) {
                return area_cfg.name;
            }
        }

        // 规则4: 描述文本正则匹配
        for (const auto& pre : area_cfg.compiled_pattern_re) {
            if (std::regex_search(desc, pre)) {
                return area_cfg.name;
            }
        }
    }

    // 所有规则都未命中 → 归入"未分类"
    return "\u672a\u5206\u7c7b"; // "未分类"
}

void classify_records(SettlementData& data, const ClassifyRules& rules) {
    double department_ratio = rules.department_ratio;

    // 逐条分类
    for (auto& record : data.all_records) {
        std::string area_name = classify_one(record, rules.areas);
        record.area = area_name;

        // 确保区域存在
        auto it = data.areas.find(area_name);
        if (it == data.areas.end()) {
            data.areas.emplace(area_name, AreaData(area_name));
            it = data.areas.find(area_name);
        }
        it->second.add_record(record);
    }

    // 计算各区域小计
    for (auto& pair : data.areas) {
        pair.second.calculate(department_ratio);
    }

    data.calculate_totals();
}

} // namespace xingda