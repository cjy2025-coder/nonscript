#pragma once
#include "error.h"
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <sstream>
#ifdef _WIN32
// windows.h 定义的宏会污染枚举名，push/pop 保护
#pragma push_macro("ERROR")
#pragma push_macro("VOID")
#pragma push_macro("TRUE")
#pragma push_macro("FALSE")
#pragma push_macro("CONST")
#pragma push_macro("THIS")
#pragma push_macro("DELETE")
#pragma push_macro("IN")
#pragma push_macro("OUT")
#pragma push_macro("NEAR")
#pragma push_macro("FAR")
#undef ERROR
#undef VOID
#undef TRUE
#undef FALSE
#undef CONST
#undef THIS
#undef DELETE
#undef IN
#undef OUT
#undef NEAR
#undef FAR
#include <windows.h>
#pragma pop_macro("FAR")
#pragma pop_macro("NEAR")
#pragma pop_macro("OUT")
#pragma pop_macro("IN")
#pragma pop_macro("DELETE")
#pragma pop_macro("THIS")
#pragma pop_macro("CONST")
#pragma pop_macro("FALSE")
#pragma pop_macro("TRUE")
#pragma pop_macro("VOID")
#pragma pop_macro("ERROR")
#endif

namespace ns
{
    // Windows 控制台颜色
    enum class ConsoleColor
    {
        RED = 12,
        GREEN = 10,
        YELLOW = 14,
        BLUE = 9,
        CYAN = 11,
        MAGENTA = 13,
        WHITE = 7,
        GRAY = 8,
    };

    class ErrorManager
    {
    private:
        std::vector<ComilerError> errors_;
        std::vector<std::string> source_lines_cache_;
        int error_count_ = 0;
        int warning_count_ = 0;
        bool has_reported_ = false;  // 同一轮中是否已报错（防级联）

#ifdef _WIN32
        void set_console_color(ConsoleColor color) const
        {
            HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleTextAttribute(h, static_cast<WORD>(color));
        }
#else
        void set_console_color(ConsoleColor) const {}
#endif

        // 安全获取源码行
        std::string get_line(const std::string &source, int line_num) const
        {
            if (line_num < 1) return "";
            int current = 1;
            for (size_t i = 0; i < source.size(); i++)
            {
                if (current == line_num)
                {
                    size_t end = source.find('\n', i);
                    if (end == std::string::npos) end = source.size();
                    while (end > i && (source[end-1] == '\r' || source[end-1] == '\n'))
                        end--;
                    return source.substr(i, end - i);
                }
                if (source[i] == '\n') current++;
            }
            return "";
        }

        // 构建脱字符标注行
        std::string build_caret(int col, const std::string &label, Severity sev) const
        {
            std::string s;
            if (col > 1) s.append(col - 1, ' ');
            s += (sev == Severity::_ERROR) ? '^' : (sev == Severity::_WARNING) ? '-' : '~';
            if (!label.empty())
                s += " " + label;
            return s;
        }

        // 美化单条错误
        std::string format_error(const ComilerError &err, const std::string &source) const
        {
            std::ostringstream os;
            auto loc = err.getLocation();
            auto code = err.getCode();

            // 第一行: error[E1001]: label
            if (err.getSeverity() == Severity::_ERROR) os << "error";
            else if (err.getSeverity() == Severity::_WARNING) os << "warning";
            else if (err.getSeverity() == Severity::_NOTE) os << "note";
            else os << "help";
            os << "[E" << static_cast<int>(code) << "]: ";
            os << error_code_label(code) << "\n";

            // 第二行:   --> file:line:col
            os << "  --> " << loc.filename << ":" << loc.line << ":" << loc.col << "\n";
            os << "   |\n";

            // 第三行: 行号 | 源码
            std::string src_line = get_line(source, loc.line);
            os << " " << loc.line << " | " << src_line << "\n";

            // 第四行:   | 脱字符 + 详细消息
            os << "   | " << build_caret(loc.col, err.getMessage(), err.getSeverity()) << "\n";

            return os.str();
        }

    public:
        void setSource(const std::string &source)
        {
            source_lines_cache_.clear();
            size_t pos = 0;
            while (pos < source.size())
            {
                size_t next = source.find('\n', pos);
                if (next == std::string::npos) next = source.size();
                std::string line = source.substr(pos, next - pos);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                source_lines_cache_.push_back(line);
                pos = next + 1;
            }
        }

        void setReported(bool r) { has_reported_ = r; }
        bool hasReported() const { return has_reported_; }

        // === 新统一接口（纯消息，不附加 code_line） ===
        void syntaxError(ErrorCode code, const std::string &msg,
                         const sourceLocation &location)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, msg, location, code, Severity::_ERROR);
            has_reported_ = true;
            error_count_++;
        }

        void semanticError(ErrorCode code, const std::string &msg,
                           const sourceLocation &location)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SEMANTIC_ERROR, msg, location, code, Severity::_ERROR);
            has_reported_ = true;
            error_count_++;
        }

        void warning(ErrorCode code, const std::string &msg,
                     const sourceLocation &location)
        {
            errors_.emplace_back(ErrorType::SEMANTIC_ERROR, msg, location, code, Severity::_WARNING);
            warning_count_++;
        }

        void note(const std::string &msg)
        {
            sourceLocation loc = {"", 0, 0};
            errors_.emplace_back(ErrorType::SEMANTIC_ERROR, msg, loc, ErrorCode::UNEXPECTED_TOKEN, Severity::_NOTE);
        }

        // === 向后兼容接口（保持旧 msg + code_line 格式，用于 whats() 旧路径） ===
        void emit_undefined_symbol_error(sourceLocation loc, const std::string &sym, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SEMANTIC_ERROR, sym + " is not defined" + ctx, loc, ErrorCode::UNDEFINED_SYMBOL, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_unknown_type_error(sourceLocation loc, const std::string &type, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SEMANTIC_ERROR, "can't find type: " + type + ctx, loc, ErrorCode::UNKNOWN_TYPE, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_redefined_symbol_error(sourceLocation loc, const std::string &sym, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SEMANTIC_ERROR, sym + " has already been defined" + ctx, loc, ErrorCode::REDEFINED_SYMBOL, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_type_not_match_error(sourceLocation loc, const std::string &cur, const std::string &exp, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SEMANTIC_ERROR, "can't convert type '" + cur + "' to type '" + exp + "'" + ctx, loc, ErrorCode::TYPE_NOT_MATCH, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_miss_token_error(sourceLocation loc, const std::string &expect, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "miss '" + expect + "'" + ctx, loc, ErrorCode::MISS_TOKEN, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_unexpected_token_error(sourceLocation loc, const std::string &tok, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "unexpected '" + tok + "'" + ctx, loc, ErrorCode::UNEXPECTED_TOKEN, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_miss_expression_error(sourceLocation sl, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "miss an expression after '='" + ctx, sl, ErrorCode::MISS_EXPRESSION, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_expect_but_got_error(sourceLocation loc, const std::string &exp, const std::string &got, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "expect '" + exp + "' but got '" + got + "'" + ctx, loc, ErrorCode::EXPECT_BUT_GOT, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_external_func_miss_return_type_error(sourceLocation sl, const std::string &name, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "external function '" + name + "' must declare return type" + ctx, sl, ErrorCode::EXTERNAL_FUNC_MISS_RETURN, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_miss_class_name_error(sourceLocation sl, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "expect a class name after 'class'" + ctx, sl, ErrorCode::CLASS_NAME_MISSING, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_miss_parent_class_name_error(sourceLocation sl, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "expect a class name as parent" + ctx, sl, ErrorCode::PARENT_CLASS_NAME_MISSING, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_class_constructor_error(sourceLocation sl, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "constructor function doesn't need a return type" + ctx, sl, ErrorCode::CLASS_CONSTRUCTOR_ERROR, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_invalid_class_member_error(sourceLocation sl, const std::string &tok, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "only function or const/variable member is allowed in class body, but got '" + tok + "'" + ctx, sl, ErrorCode::INVALID_CLASS_MEMBER, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_invalid_token_after_contorller_for_class(sourceLocation sl, const std::string &ctrl, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "only function or const/variable member declaration is allowed after '" + ctrl + "'" + ctx, sl, ErrorCode::INVALID_TOKEN_AFTER_CONTROLLER, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_miss_object_member_error(sourceLocation sl, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "expect a member name after '.'" + ctx, sl, ErrorCode::MISS_OBJECT_MEMBER, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_func_param_miss_type_error(sourceLocation sl, const std::string &tok, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "expect a type declaration before function parameter instead of got '" + tok + "'" + ctx, sl, ErrorCode::FUNC_PARAM_MISS_TYPE, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_func_param_miss_name_error(sourceLocation sl, const std::string &tok, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "expect a parameter name after type '" + tok + "'" + ctx, sl, ErrorCode::FUNC_PARAM_MISS_NAME, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_func_name_missing_error(sourceLocation sl, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "expect a function name after 'func'" + ctx, sl, ErrorCode::FUNC_NAME_MISSING, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_func_return_type_missing_error(sourceLocation sl, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "a return type declaration is needed after '->'" + ctx, sl, ErrorCode::FUNC_RETURN_TYPE_MISSING, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_ident_missing_error(sourceLocation sl, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "expect an identifier" + ctx, sl, ErrorCode::IDENT_MISSING, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_ident_type_missing_error(sourceLocation sl, const std::string &ident, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "expect a type after '" + ident + ":'" + ctx, sl, ErrorCode::IDENT_TYPE_MISSING, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }
        void emit_contrant_not_init_error(sourceLocation sl, const std::string &ident, const std::string &ctx)
        {
            if (has_reported_) return;
            errors_.emplace_back(ErrorType::SYNTAX_ERROR, "const value '" + ident + "' must be init" + ctx, sl, ErrorCode::CONST_NOT_INIT, Severity::_ERROR);
            has_reported_ = true; error_count_++;
        }

        void report(ComilerError error)
        {
            if (has_reported_) return;
            if (error.getSeverity() == Severity::_ERROR) error_count_++;
            else if (error.getSeverity() == Severity::_WARNING) warning_count_++;
            errors_.push_back(error);
            if (error.getSeverity() == Severity::_ERROR)
                has_reported_ = true;
        }
        void report(ErrorType type_, const std::string &msg_, const sourceLocation &location_)
        {
            if (has_reported_) return;
            errors_.emplace_back(type_, msg_, location_);
            error_count_++;
            has_reported_ = true;
        }

        const std::vector<ComilerError> &getAll() const { return errors_; }
        void clear()
        {
            errors_.clear();
            error_count_ = 0;
            warning_count_ = 0;
            has_reported_ = false;
        }
        bool has() const { return error_count_ > 0; }
        int errorCount() const { return error_count_; }
        int warningCount() const { return warning_count_; }

        // 美化输出（供 main.cpp 使用）
        std::string prettyPrint(const std::string &source)
        {
            if (errors_.empty()) return "";

            std::ostringstream os;
            for (const auto &err : errors_)
                if (err.getSeverity() == Severity::_ERROR)
                    os << format_error(err, source);
            for (const auto &err : errors_)
                if (err.getSeverity() == Severity::_WARNING)
                    os << format_error(err, source);
            for (const auto &err : errors_)
                if (err.getSeverity() == Severity::_NOTE || err.getSeverity() == Severity::_HELP)
                    os << format_error(err, source);

            if (error_count_ > 0)
            {
                os << "\n";
                if (error_count_ == 1) os << "aborting due to 1 previous error";
                else os << "aborting due to " << error_count_ << " previous errors";
                if (warning_count_ > 0) os << " (" << warning_count_ << " warnings)";
                os << "\n";
            }
            else if (warning_count_ > 0)
                os << "\n" << warning_count_ << " warning(s) generated.\n";

            return os.str();
        }

        // 旧 whats() 保持兼容
        std::string whats() const
        {
            std::string result;
            for (size_t i = 0; i < errors_.size(); i++)
            {
                result += errors_[i].what();
                if (i != errors_.size() - 1) result += "\n";
            }
            return result;
        }

        int num() const { return static_cast<int>(errors_.size()); }
    };
}