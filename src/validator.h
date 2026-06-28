#ifndef VALIDATOR_H
#define VALIDATOR_H

#include <string>
#include "models.h"

namespace xingda {

/// 执行金额闭环校验。返回 true 表示通过（偏差 < 5%）
bool validate_amounts(SettlementData& data);

/// 生成校验摘要文本
std::string generate_validation_summary(const SettlementData& data);

} // namespace xingda

#endif // VALIDATOR_H