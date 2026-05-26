#pragma once
#include "error.h"
#include <vector>
#include <string>
#include <iostream>

namespace ns
{
    class ErrorManager
    {
    private:
        std::vector<ComilerError> errors_;

    public:
        void
        emit_undefined_symbol_error(sourceLocation source_location,
                                    const std::string &symbol_name,
                                    const std::string &code_context)
        {
            ComilerError ce = ComilerError(symbol_name + " is not defined" + code_context, source_location);
            errors_.emplace_back(ce);
        }
        void emit_unknown_type_error(sourceLocation source_location,
                                     const std::string &type,
                                     const std::string &code_context)
        {
            ComilerError ce = ComilerError("can't find type : " + type + code_context, source_location);
            errors_.emplace_back(ce);
        }
        void emit_redefined_symbol_error(sourceLocation source_location,
                                         const std::string &symbol_name,
                                         const std::string &code_context)
        {
            ComilerError ce = ComilerError(symbol_name + " has aready been defined" + code_context, source_location);
            errors_.emplace_back(ce);
        }
        void emit_type_not_match_error(sourceLocation source_location,
                                       const std::string &current_type,
                                       const std::string &expect_type,
                                       const std::string &code_context)
        {
            ComilerError ce = ComilerError("can't convert type '" + current_type + "' to type '" + expect_type + "'" + code_context, source_location);
            errors_.emplace_back(ce);
        }
        void emit_miss_token_error(sourceLocation source_location,
                                   const std::string &expect_token,
                                   const std::string &code_context)
        {
            ComilerError ce = ComilerError("miss '" + expect_token + "'" + code_context, source_location);
            errors_.emplace_back(ce);
        }
        void emit_unexpected_token_error(sourceLocation source_location,
                                         const std::string &current_unexpected_token,
                                         const std::string &code_context)
        {
            ComilerError ce = ComilerError("unexpected '" + current_unexpected_token + "'" + code_context, source_location);
            errors_.emplace_back(ce);
        }
        void emit_miss_expression_error(sourceLocation sl, const std::string &code_context)
        {
            ComilerError ce = ComilerError("miss an expression after '='" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_expect_but_got_error(sourceLocation source_location,
                                       const std::string &expected_token,
                                       const std::string &current_token,
                                       const std::string &code_context)
        {
            ComilerError ce = ComilerError("expect '" + expected_token + "' but got '" + current_token + "'" + code_context, source_location);
            errors_.emplace_back(ce);
        }
        void emit_external_func_miss_return_type_error(sourceLocation sl,
                                                       const std::string &func_name,
                                                       const std::string &code_context)
        {
            ComilerError ce = ComilerError("external function '" + func_name + "' must decalare return type" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_miss_class_name_error(sourceLocation sl, const std::string &code_context)
        {
            ComilerError ce = ComilerError("expect a class name after 'class'" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_miss_parent_class_name_error(sourceLocation sl, const std::string &code_context)
        {
            ComilerError ce = ComilerError("expect a class name as parent" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_invalid_class_member_error(sourceLocation sl,
                                             const std::string &invalid_token,
                                             const std::string &code_context)
        {
            ComilerError ce = ComilerError("only function or const/variable member is allowed in class body, but got '" + invalid_token + "'" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_invalid_token_after_contorller_for_class(sourceLocation sl,
                                                           const std::string &controller_token,
                                                           const std::string &code_context)
        {
            ComilerError ce = ComilerError("only function or const/variable member declaration is allowed after '" + controller_token + "'" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_miss_object_member_error(sourceLocation sl, const std::string &code_context)
        {
            ComilerError ce = ComilerError("expect a member name after '.'" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_func_param_miss_type_error(sourceLocation sl,
                                             const std::string &current_token,
                                             const std::string &code_context)
        {
            ComilerError ce = ComilerError("expect a type decalaration before function parameter instead of got '" + current_token + "'" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_func_param_miss_name_error(sourceLocation sl,
                                             const std::string &current_token,
                                             const std::string &code_context)
        {
            ComilerError ce = ComilerError("expect a parameter name after type '" + current_token + "'" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_func_name_missing_error(sourceLocation sl, const std::string &code_context)
        {
            ComilerError ce = ComilerError("expect a function name after 'func'" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_func_return_type_missing_error(sourceLocation sl, const std::string &code_context)
        {
            ComilerError ce = ComilerError("a return type decalaration is needed after '->'" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_ident_missing_error(sourceLocation sl, const std::string &code_context)
        {
            ComilerError ce = ComilerError("expect an ident" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_ident_type_missing_error(sourceLocation sl, const std::string &ident, const std::string &code_context)
        {
            ComilerError ce = ComilerError("expect a type after '" + ident + ":'" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void emit_contrant_not_init_error(sourceLocation sl, const std::string &ident, const std::string &code_context)
        {
            ComilerError ce = ComilerError("const value '" + ident + "' must be init" + code_context, sl);
            errors_.emplace_back(ce);
        }
        void report(ComilerError error)
        {
            errors_.push_back(error);
        }
        void report(ErrorType type_, const std::string &msg_, const sourceLocation &location_)
        {
            errors_.push_back(ComilerError(type_, msg_, location_));
        }
        const std::vector<ComilerError> &getAll() const
        {
            return errors_;
        }
        void clear()
        {
            errors_.clear();
        }
        bool has() const
        {
            return !errors_.empty();
        }
        std::string whats() const
        {
            std::string result;
            for (int i = 0; i < errors_.size(); i++)
            {
                result += errors_[i].what();

                if (i != errors_.size() - 1)
                    result += "\n";
            }
            return result;
        }

        int num() const
        {
            return errors_.size();
        }
    };
}