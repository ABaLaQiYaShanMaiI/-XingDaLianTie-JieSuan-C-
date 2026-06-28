#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace xingda {

/// 基础异常类
class XingDaError : public std::runtime_error {
public:
    explicit XingDaError(const std::string& msg) : std::runtime_error(msg) {}
};

/// PDF 解析异常
class ParseError : public XingDaError {
public:
    explicit ParseError(const std::string& msg) : XingDaError("PDF解析失败: " + msg) {}
};

/// 配置异常
class ConfigError : public XingDaError {
public:
    explicit ConfigError(const std::string& msg) : XingDaError("配置错误: " + msg) {}
};

/// Excel 写入异常
class ExcelWriteError : public XingDaError {
public:
    explicit ExcelWriteError(const std::string& msg) : XingDaError("Excel生成失败: " + msg) {}
};

} // namespace xingda

#endif // EXCEPTIONS_H