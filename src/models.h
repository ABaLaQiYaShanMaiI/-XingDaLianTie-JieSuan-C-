#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <algorithm>
#include <cmath>

namespace xingda {

/// 考核记录
struct AssessmentRecord {
    int index = 0;
    std::string description;
    std::string clause;
    double amount = 0.0;
    std::string area;
    std::string remark;
    std::string parse_source;   // "table" 或 "text"
    std::vector<std::string> parse_warnings;

    AssessmentRecord() = default;
    AssessmentRecord(int idx, std::string desc, std::string cls, double amt)
        : index(idx), description(std::move(desc)), clause(std::move(cls)), amount(amt) {}
};

/// 作业区数据
class AreaData {
public:
    std::string name;
    std::vector<AssessmentRecord> records;
    double subtotal = 0.0;
    double dept_amount = 0.0;

    explicit AreaData(std::string n) : name(std::move(n)) {}

    void add_record(const AssessmentRecord& record) {
        records.push_back(record);
    }

    void calculate(double ratio = 0.01) {
        subtotal = 0.0;
        for (const auto& r : records) {
            subtotal += r.amount;
        }
        dept_amount = std::round(subtotal * ratio * 100.0) / 100.0;
        // 按考核金额降序排列
        std::sort(records.begin(), records.end(),
            [](const AssessmentRecord& a, const AssessmentRecord& b) {
                return a.amount > b.amount;
            });
    }
};

/// 结算单完整数据
class SettlementData {
public:
    // 合同基本信息
    std::string contract_no;
    std::string contract_name;
    std::string work_period;
    std::string month_label;

    // 费用信息
    double work_fee = 0.0;
    double total_assessment = 0.0;
    double total_reward = 0.0;
    double settlement_amount = 0.0;

    // 考核明细
    std::vector<AssessmentRecord> all_records;
    std::map<std::string, AreaData> areas;

    // 校验信息
    std::optional<double> pdf_stated_total;
    bool amount_match = true;
    double amount_deviation_pct = 0.0;

    // 元信息
    std::string pdf_path;
    bool from_ocr = false;
    std::string raw_text;

    void calculate_totals() {
        total_assessment = 0.0;
        for (auto& pair : areas) {
            total_assessment += pair.second.subtotal;
        }
    }

    double get_settlement_amount() const {
        return work_fee - total_assessment + total_reward;
    }

    double settlement_amount_or_computed() const {
        if (settlement_amount > 0.0) {
            return settlement_amount;
        }
        return get_settlement_amount();
    }
};

} // namespace xingda

#endif // MODELS_H