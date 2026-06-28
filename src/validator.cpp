#include "validator.h"
#include <sstream>
#include <cmath>
#include <iomanip>

namespace xingda {

static const double DEVIATION_WARN_THRESHOLD = 0.05;

bool validate_amounts(SettlementData& data) {
    double extracted_total = data.total_assessment;
    auto pdf_stated = data.pdf_stated_total;

    if (!pdf_stated.has_value()) {
        return true; // 无法校验时不阻止生成
    }

    double deviation = std::abs(extracted_total - pdf_stated.value());
    double deviation_pct = (pdf_stated.value() > 0.0) ? (deviation / pdf_stated.value()) : 0.0;

    data.amount_deviation_pct = deviation_pct;

    if (deviation_pct == 0.0) {
        data.amount_match = true;
        return true;
    } else if (deviation_pct < DEVIATION_WARN_THRESHOLD) {
        data.amount_match = true;
        return true;
    } else {
        data.amount_match = false;
        return false;
    }
}

static std::string fmt_amount(double v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << v;
    std::string s = ss.str();
    // 添加千位分隔符
    int pos = s.find('.');
    if (pos == std::string::npos) pos = s.size();
    for (int i = pos - 3; i > 0; i -= 3) {
        s.insert(i, ",");
    }
    return s;
}

std::string generate_validation_summary(const SettlementData& data) {
    std::ostringstream ss;

    ss << "==================================================" << std::endl;
    ss << "\u7ed3\u7b97\u5355\u6821\u9a8c\u6458\u8981" << std::endl; // "结算单校验摘要"
    ss << "==================================================" << std::endl;

    if (!data.contract_no.empty())
        ss << "\u5408\u540c\u7f16\u53f7: " << data.contract_no << std::endl; // "合同编号"
    if (!data.contract_name.empty())
        ss << "\u5408\u540c\u540d\u79f0: " << data.contract_name << std::endl; // "合同名称"
    if (!data.work_period.empty())
        ss << "\u4f5c\u4e1a\u65f6\u95f4: " << data.work_period << std::endl; // "作业时间"

    ss << std::endl;
    ss << "\u8003\u6838\u660e\u7ec6: " << data.all_records.size() << " \u6761\u8bb0\u5f55" << std::endl; // "考核明细: X 条记录"
    ss << "\u7a0b\u5e8f\u63d0\u53d6\u5408\u8ba1: \u00a5" << fmt_amount(data.total_assessment) << std::endl; // "程序提取合计: ¥"

    if (data.pdf_stated_total.has_value()) {
        ss << "PDF \u58f0\u660e\u5408\u8ba1: \u00a5" << fmt_amount(data.pdf_stated_total.value()) << std::endl; // "PDF 声明合计: ¥"
        ss << "\u504f\u5dee: " << std::fixed << std::setprecision(2) << (data.amount_deviation_pct * 100.0)
           << "% " << (data.amount_match ? "\u2705 \u901a\u8fc7" : "\u274c \u5931\u8d25") << std::endl; // "✅ 通过" / "❌ 失败"
    }

    if (data.total_reward > 0)
        ss << "\u5609\u5956\u91d1\u989d: \u00a5" << fmt_amount(data.total_reward) << std::endl; // "嘉奖金额: ¥"
    if (data.work_fee > 0) {
        ss << "\u4f5c\u4e1a\u8d39\u7528: \u00a5" << fmt_amount(data.work_fee) << std::endl; // "作业费用: ¥"
        double settlement = data.get_settlement_amount();
        ss << "\u5f53\u6708\u7ed3\u7b97\u8d39\u7528: \u00a5" << fmt_amount(settlement) << std::endl; // "当月结算费用: ¥"
    }

    ss << std::endl << "\u533a\u57df\u660e\u7ec6:" << std::endl; // "区域明细:"
    for (const auto& pair : data.areas) {
        const auto& area_data = pair.second;
        if (area_data.records.empty()) continue;
        ss << "  " << area_data.name << ": "
           << area_data.records.size() << " \u6761, " // "条"
           << "\u5c0f\u8ba1 \u00a5" << fmt_amount(area_data.subtotal) << ", " // "小计 ¥"
           << "\u4e8b\u4e1a\u90e8 \u00a5" << fmt_amount(area_data.dept_amount) << std::endl; // "事业部 ¥"
    }

    return ss.str();
}

} // namespace xingda