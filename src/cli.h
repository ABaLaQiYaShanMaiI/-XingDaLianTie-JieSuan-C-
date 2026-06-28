#ifndef CLI_H
#define CLI_H

#include <string>
#include <optional>
#include "config.h"

namespace xingda {

/// 处理单个 PDF 文件，返回生成的输出文件路径
std::string process_pdf_core(
    const std::string& pdf_path,
    const std::string& output_dir,
    const std::optional<std::string>& rules_path,
    bool validate_only,
    bool dump_text,
    bool include_summary,
    bool enable_ocr,
    bool summary_only,
    bool no_merge,
    const ParserConfig& parser_config,
    const std::optional<std::string>& style_name
);

} // namespace xingda

#endif // CLI_H