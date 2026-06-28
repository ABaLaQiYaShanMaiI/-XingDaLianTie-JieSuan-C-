#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "models.h"
#include "config.h"

namespace xingda {

/// 解析 PDF 结算单，返回 SettlementData。
/// @throws ParseError 当 PDF 完全无法解析时
/// @throws FileNotFoundError 当 PDF 文件不存在时
SettlementData parse_pdf(const std::string& pdf_path, const ParserConfig& pconfig = ParserConfig::default_config());

} // namespace xingda

#endif // PARSER_H