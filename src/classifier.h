#ifndef CLASSIFIER_H
#define CLASSIFIER_H

#include "models.h"
#include "config.h"

namespace xingda {

/// 对所有考核记录按配置规则分类
void classify_records(SettlementData& data, const ClassifyRules& rules);

} // namespace xingda

#endif // CLASSIFIER_H