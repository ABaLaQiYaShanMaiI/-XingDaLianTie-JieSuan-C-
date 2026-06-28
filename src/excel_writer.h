#ifndef EXCEL_WRITER_H
#define EXCEL_WRITER_H

#include <string>
#include <vector>
#include "models.h"
#include "config.h"

namespace xingda {

/// 生成 Excel 结算单明细文件
void generate_excel(
    const SettlementData& data,
    const std::string& output_path,
    const std::vector<std::string>& area_order,
    const ExcelStyle& style,
    bool include_summary = true,
    bool summary_only = false
);

} // namespace xingda

#endif // EXCEL_WRITER_H