#include "./frontend/parser.h"

namespace ns
{
    void Parser::setLexer(Lexer *lexer)
    {
        mLexer = lexer;
    }
    bool Parser::currentIs(TokenType type) const
    {
        return current.getType() == type;
    }
    Priority Parser::get_precedence() const
    {
        auto tt = current.getType();
        switch (tt)
        {
        case TokenType::ASSIGN:
            return Priority::ASSIGN;
        case TokenType::EQ:
        case TokenType::NEQ:
        case TokenType::AND:
        case TokenType::OR:
            return Priority::EQUAL;
        case TokenType::LT:
        case TokenType::GT:
        case TokenType::LTE:
        case TokenType::GTE:
            return Priority::LESSGREATER;
        case TokenType::ADD:
        case TokenType::MINUS:
            return Priority::ADD_;
        case TokenType::MUL:
        case TokenType::DIV:
        case TokenType::MOD:
            return Priority::PRODUCT;
        case TokenType::POINT:
            return Priority::POINT;
        case TokenType::LPAREN:
            return Priority::CALL;
        case TokenType::LBRACKET:
            return Priority::INDEX;
        default:
            return Priority::LOWEST;
        }
    }
    std::unique_ptr<Program> Parser::parse()
    {
        std::unique_ptr<Program> program(new Program);
        while (current.getType() != TokenType::END && current.getType() != TokenType::ILLEGAL)
        {
            auto stmt = parse_statement();
            if (stmt == nullptr)
            {
                return nullptr;
            }
            if (typeid(*stmt) == typeid(ShortStatement))
            {
                em_->emit_unexpected_token_error(CURRENT_LOCATION, stmt->toString(), get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            program->append(stmt.release());
        }
        if (current.getType() == TokenType::ILLEGAL)
        {
            em_->emit_unexpected_token_error(CURRENT_LOCATION, current.getLiteral(), get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        return program;
    }
    std::unique_ptr<ForLoop> Parser::parse_for_loop()
    {
        auto loop = std::make_unique<ForLoop>();
        bool is_external_variable = true;
        advance(); // 跳过 for 关键字
        if (!match(TokenType::LPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "(", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过左括号

        // 如果出现 var 关键字，则是for-loop 局部变量，否则是外部捕获变量
        // 不能出现 const 常量声明
        if (match(TokenType::VAR))
        {
            is_external_variable = false;
            advance(); // 跳过 var 关键字
        }
        while (match(TokenType::IDENT))
        {
            auto ident = parse_ident_expression(); // ident不可能为空，所以不用检查
            // 显式申明了类型
            if (match(TokenType::COLON))
            {
                // 跳过冒号
                advance();
                // 此时需要一个标识符来充当类型
                if (!match(TokenType::IDENT))
                {
                    em_->emit_ident_type_missing_error(current.getLocation(), ident->getLiteral(), get_code_line_string(CURRENT_LOCATION));
                    return nullptr;
                }
                
                // 将显示声明的类型写入标识符 ast 结点
                std::string vt = current.getLiteral();
                const TypeInfo *const ti = typeManager::find(vt);

                // 从类型表中检查当前类型是否合法
                if (!ti)
                {
                    em_->emit_unknown_type_error(current.getLocation(), vt, get_code_line_string(CURRENT_LOCATION));
                    return nullptr;
                }
                ident->setType(ti->baseType);
                advance(); // 跳过声明的类型标识符
            }
            // 进行了初始化
            if(match(TokenType::ASSIGN)){
                advance(); // 跳过等号;
                auto rvalue = parse_expression(Priority::LOWEST);
                if (!rvalue) // 初始化表达式有错
                {
                    return nullptr;
                }
                auto lv = std::make_unique<LoopVariable>();
                lv->external_variable = is_external_variable;
                lv->var = std::move(ident);
                lv->value = std::move(rvalue);
                loop->addVariable(std::move(lv));
            }
            else if(!is_external_variable) // 没有初始化，但是也并不是外部变量
            {
                em_->emit_miss_token_error(current.getLocation(), "=", get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            
            if(match(TokenType::COMMA)){
                advance();
            }
            else if(!match(TokenType::SEMICOLON)){
                em_->emit_miss_token_error(current.getLocation(),",",get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
        }
        // if (!match(TokenType::SEMICOLON))
        // {
        //     em_->emit_miss_token_error(current.getLocation(), ";", get_code_line_string(CURRENT_LOCATION));
        //     return nullptr;
        // }

        advance(); // 跳过第一个分号

        // 下面逻辑将判断当前 token 是否是分号
        // 1）直接遇到了第二个分号，证明循环条件为空
        //    此时保持条件为 nullptr
        //    后续处理时通过判断指针是否为空，就可以判别是否声明了循环条件
        // 2）否则直接尝试解析循环条件
        if(!match(TokenType::SEMICOLON)){
            auto condition = parse_expression(Priority::LOWEST); // 解析条件
            if (!condition)
            {
                return nullptr;
            }
            loop->setConditon(std::move(condition));
            if (!match(TokenType::SEMICOLON))
            {
                em_->emit_miss_token_error(current.getLocation(), ";", get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
        }
        advance(); // 跳过第二个分号

        // 下面逻辑将判断当前 token 是否是右括号
        // 1）直接遇到了右括号，证明步进为空
        //    此时保持步进为 nullptr
        //    后续处理时通过判断指针是否为空，就可以判别是否声明了步进
        // 2）否则直接尝试解析步进
        if (!match(TokenType::RPAREN)){
            while (1)
            {
                auto e = parse_expression(Priority::LOWEST);
                if (!e)
                {
                    return nullptr;
                }
                loop->addAction(std::move(e));
                if (match(TokenType::RPAREN) || match(TokenType::END)) // 判断步进部分是否结束，遇到终止符时强制结束
                {
                    break;
                }

                // 进入此处，表明步进声明并未结束
                // 那么此时将转到处理下一个步进的操作
                // 但是步进操作之间必须用 , 分隔
                if (!match(TokenType::COMMA))
                {
                    em_->emit_miss_token_error(current.getLocation(), ",", get_code_line_string(CURRENT_LOCATION));
                }
                advance(); // 跳过逗号
            }
        }
        if (!match(TokenType::RPAREN)) // 不是右括号，那么证明右括号缺失
        {
            em_->emit_miss_token_error(current.getLocation(), ")", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过右括号
        auto body = parse_blockstatement(1);
        if (!body)
        {
            return nullptr;
        }
        loop->setBody(std::move(body));
        if (match(TokenType::SEMICOLON))
        {
            advance(); // 跳过可能的尾随分号
        }
        return loop;
    }
    std::unique_ptr<TryCatchStatement> Parser::parse_try_catch_statement()
    {
        auto stmt = std::make_unique<TryCatchStatement>();
        advance(); // 跳过try关键字
        auto try_body = parse_blockstatement(0);
        if (!try_body)
        {
            return nullptr;
        }
        if (!match(TokenType::CATCH))
        {
            em_->emit_miss_token_error(current.getLocation(), "catch", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过catch关键字
        if (!match(TokenType::LPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "(", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过左括号
        if (!match(TokenType::IDENT))
        {
            return nullptr;
        }
        auto _exception = parse_ident_expression();
        if (!_exception)
        {
            return nullptr;
        }
        if (!match(TokenType::RPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), ")", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过右括号
        auto catch_body = parse_blockstatement(0);
        if (!catch_body)
        {
            return nullptr;
        }
        stmt->setException(std::move(_exception));
        stmt->setTryBody(std::move(try_body));
        stmt->setExceptionBody(std::move(catch_body));
        return stmt;
    }
    std::unique_ptr<IndexExpression> Parser::parse_index_expression(Expression *left)
    {
        std::unique_ptr<IndexExpression> ie = std::make_unique<IndexExpression>();
        auto e = parse_expression(Priority::LOWEST);
        ie->setLeft(left);
        ie->setIndex(e.release());
        if (!match(TokenType::RBRACKET))
        {
            em_->emit_miss_token_error(current.getLocation(), "[", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance();
        return ie;
    }
    std::vector<std::unique_ptr<Expression>> Parser::parse_expression_list(TokenType end, int &error)
    {
        std::vector<std::unique_ptr<Expression>> args;
        if (next.getType() == end)
        {
            advance(); // 跳至右中括号
            advance(); // 跳至下一Token
            return {};
        }
        advance();
        auto arg = parse_expression(Priority::LOWEST);
        args.emplace_back(arg.release());
        while (current.getType() == TokenType::COMMA)
        {
            advance();
            args.emplace_back(parse_expression(Priority::LOWEST).release());
        }
        if (!match(end))
        {
            em_->emit_miss_token_error(current.getLocation(), getName(end), get_code_line_string(CURRENT_LOCATION));
            error = 1;
            return {};
        }
        advance();
        return args;
    }

    std::unique_ptr<ArrayLiteral> Parser::parse_array_expression()
    {
        std::unique_ptr<ArrayLiteral> arr(new ArrayLiteral(current));
        int error = 0;
        auto elems = parse_expression_list(TokenType::RBRACKET, error);
        if (error)
        { // 数组元素解析出错
            return nullptr;
        }
        arr->setElems(elems);
        return arr;
    }

    std::vector<std::shared_ptr<FuncParam>> Parser::parse_func_params(int &error)
    { // 进来时token为左括号
        std::vector<std::shared_ptr<FuncParam>> params;
        advance();                                    // 跳过左括号
        if (current.getType() == TokenType::RPAREN) // 函数参数为空
        {
            advance();
            return {};
        }

        do
        {
            if (!match(TokenType::IDENT))
            {
                em_->emit_func_param_miss_type_error(current.getLocation(), current.getLiteral(), get_code_line_string(CURRENT_LOCATION));
                error = 1;
                return {};
            }
            auto &&token = current;
            auto type = parse_ident_expression();
            auto _ = typeManager::find(type->getLiteral());
            if (!_)
            { // 类型不存在
                error = 1;
                em_->emit_unknown_type_error(token.getLocation(), token.getLiteral(), get_code_line_string(token.getLocation()));
                return {};
            }
            if (current.getType() != TokenType::IDENT)
            { // 类型后没有形参名
                error = 1;
                em_->emit_func_param_miss_name_error(token.getLocation(), token.getLiteral(), get_code_line_string(token.getLocation()));
                return {};
            }
            auto ident = parse_ident_expression();
            ident->setType(_->baseType);
            auto param = std::make_shared<FuncParam>();
            param->name = std::move(ident);
            if (current.getType() == TokenType::ASSIGN)
            {              // 函数参数有初始值
                advance(); // 跳过等号
                auto val = parse_expression(Priority::LOWEST);
                if (!val)
                {
                    em_->emit_miss_expression_error(current.getLocation(), get_code_line_string(CURRENT_LOCATION));
                    error = 1;
                    return {};
                }
                param->init = std::move(val);
            }
            params.emplace_back(param);
        } while (current.getType() == TokenType::COMMA && (advance(), true));
        if (!match(TokenType::RPAREN))
        { // 希望遇到右括号以结束参数申明
            em_->emit_miss_token_error(current.getLocation(), ")", get_code_line_string(CURRENT_LOCATION));
            error = 1;
            return {};
        }
        advance(); // 跳过右括号
        return params;
    }

    std::unique_ptr<FuncDecl> Parser::parse_func_statement(bool is_extern_func)
    {
        advance(); // 跳过func关键字
        if (!match(TokenType::IDENT))
        { // func关键字后为函数名，应为标识符
            em_->emit_func_name_missing_error(current.getLocation(), get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        std::unique_ptr<FuncDecl> func = std::make_unique<FuncDecl>(current);
        const std::string &func_name = current.getLiteral();
        advance(); // 跳过标识符
        if (!match(TokenType::LPAREN))
        { // 判断函数名后是否有左括号
            em_->emit_miss_token_error(current.getLocation(), "(", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        int error = 0;
        auto params = parse_func_params(error);
        if (error == 1)
        { // 解析函数参数时出错
            return nullptr;
        }
        func->setParams(params);
        if (match(TokenType::ARROW))
        {
            advance(); // 跳过 ->
            if (!match(TokenType::IDENT))
            {
                em_->emit_func_return_type_missing_error(current.getLocation(), get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            const std::string &type = current.getLiteral();
            TypeInfo *const &ti = typeManager::find(type);
            if (!ti)
            {
                em_->emit_unknown_type_error(current.getLocation(), type, get_code_line_string(current.getLocation()));
                return nullptr;
            }
            func->setRetType(ti->baseType);
            advance(); // 跳过返回类型声明
        }
        // if (current_token_type() == TokenType::MINUS)
        // {
        //     advance();
        //     if (!match(TokenType::GT))
        //     {
        //         em_->emit_miss_token_error(current.getLocation(),">",get_code_line_string(CURRENT_LOCATION));
        //         return nullptr;
        //     }
        //     advance();
        //     if (!match(TokenType::IDENT))
        //     {
        //         em_->emit_func_return_type_missing_error(current.getLocation(),get_code_line_string(CURRENT_LOCATION));
        //         return nullptr;
        //     }
        //     auto token = current;
        //     auto _ = parse_ident_expression();
        //     if (!_)
        //     {
        //         return nullptr;
        //     }
        //     auto type = typeManager::find(_->getLiteral());
        //     if (!type)
        //     {
        //         em_->emit_unknown_type_error(token.getLocation(), token.getLiteral(),get_code_line_string(token.getLocation()));
        //         return nullptr;
        //     }
        //     func->setRetType(type->baseType);
        // }
        else if (is_extern_func)
        { // 是 extern 函数必须声明返回值类型
            em_->emit_external_func_miss_return_type_error(current.getLocation(), func_name, get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        if (is_extern_func)
        {
            func->mark_external();
            if (match(TokenType::SEMICOLON))
            { // 跳过可能的尾随分号
                advance();
            }
            return func;
        }
        if (!match(TokenType::LBPAREN))
        { // 希望遇到左大括号
            em_->emit_miss_token_error(current.getLocation(), "{", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        // advance(); // 跳过左大括号
        auto body = parse_blockstatement(0);
        if (body == nullptr)
        { // 函数体解析出错
            return nullptr;
        }
        func->setBody(body.release());
        // auto it = current;
        return func;
    }

    std::unique_ptr<BlockStatement> Parser::parse_blockstatement(int loop)
    {
        if (!match(TokenType::LBPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "{", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        std::unique_ptr<BlockStatement> bstmt(new BlockStatement(current));
        advance();
        while (current.getType() != TokenType::RBPAREN && current.getType() != TokenType::END)
        {
            auto stmt = parse_statement();
            if (stmt == nullptr)
                return nullptr;
            if (typeid(*stmt) == typeid(ShortStatement) && !loop) // 除了循环语句外，其它语句不能出现break,continue
            {
                em_->emit_unexpected_token_error(current.getLocation(), stmt->toString(), get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            if (stmt != nullptr)
                bstmt->append(stmt.release());
            // advance();
        }
        if (!match(TokenType::RBPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "}", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过右大括号
        return bstmt;
    }

    std::unique_ptr<ExpressionStatement> Parser::parse_expression_statement()
    {
        std::unique_ptr<ExpressionStatement> estmt(new ExpressionStatement(current));
        auto e = parse_expression(Priority::LOWEST);
        if (e == nullptr)
        {
            // if(peek().getType() != TokenType::END) advance();
            return nullptr;
        }
        estmt->setExpression(e.release());
        if (current.getType() == TokenType::SEMICOLON)
            advance();
        return estmt;
    }

    std::unique_ptr<ClassLiteral> Parser::parse_class_statement()
    {
        advance(); // 跳过class关键字
        if (!match(TokenType::IDENT))
        {
            em_->emit_miss_class_name_error(current.getLocation(), get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        std::unique_ptr<ClassLiteral> c(new ClassLiteral(current)); // 将类名传入
        typeManager::emit(current.getLiteral());                    // 注册类到类型系统
        advance();
        if (current.getType() == TokenType::EXTENDS)
        {              // 如果有父类
            advance(); // 跳过extends关键字
            if (!match(TokenType::IDENT))
            {
                em_->emit_miss_parent_class_name_error(current.getLocation(), get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            auto id = parse_ident_expression(); // 解析父类名
            if (!id)
            {
                return nullptr;
            }
            c->addBaseClass(std::move(id)); // 设置父类
            while (match(TokenType::COMMA))
            {              // 如果有多个父类
                advance(); // 跳过逗号
                if (!match(TokenType::IDENT))
                {
                    em_->emit_miss_parent_class_name_error(current.getLocation(), get_code_line_string(CURRENT_LOCATION));
                    return nullptr;
                }
                auto id = parse_ident_expression();
                if (!id)
                {
                    return nullptr;
                }
                c->addBaseClass(std::move(id));
                // advance();//跳过父类名
            }
        }
        if (!match(TokenType::LBPAREN)) // 如果不是左大括号，证明申明有误
        {
            em_->emit_miss_token_error(current.getLocation(), "{", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance();                                                                                 // 跳过左大括号
        while (current.getType() != TokenType::RBPAREN && current.getType() != TokenType::END) // 解析类成员知道结束或者源文件到头
        {
            if (current.getType() == TokenType::PRIVATE)
            { // 遇到private关键字
                // bool isStatic = false;
                auto old_token = current;
                advance();
                /*if (token.getType() == TokenType::STATIC)
                {
                    advance();
                    isStatic = true;
                }*/
                if (current.getType() == TokenType::FUNC)
                { // 遇到函数申明
                    auto stmt = parse_statement();
                    if (stmt == nullptr)
                    {
                        return nullptr;
                    }
                    c->addMember(AccessLevel::Private, MemberType::Method, std::move(stmt));
                }
                else if (match(TokenType::VAR) || match(TokenType::CONST))
                { // 遇到变量申明
                    auto stmt = parse_statement();
                    if (stmt == nullptr)
                    {
                        return nullptr;
                    }
                    c->addMember(AccessLevel::Private, MemberType::Field, std::move(stmt));
                }
                else
                {
                    em_->emit_invalid_token_after_contorller_for_class(old_token.getLocation(), "private", get_code_line_string(old_token.getLocation()));
                    return nullptr;
                }
            }
            else if (current.getType() == TokenType::PUBLIC)
            { // 遇到public关键字
                advance();
                /*if (token.getType() == TokenType::STATIC)
                {
                    advance();
                    isStatic = true;
                }*/
                if (current.getType() == TokenType::FUNC)
                { // 遇到函数申明
                    auto stmt = parse_statement();
                    if (stmt == nullptr)
                    {
                        return nullptr;
                    }
                    c->addMember(AccessLevel::Public, MemberType::Method, std::move(stmt));
                }
                else if (current.getType() == TokenType::VAR)
                { // 遇到变量申明
                    auto stmt = parse_statement();
                    if (stmt == nullptr)
                    {
                        return nullptr;
                    }
                    c->addMember(AccessLevel::Public, MemberType::Field, std::move(stmt));
                }
                else
                {
                    em_->emit_invalid_token_after_contorller_for_class(current.getLocation(), "public", get_code_line_string(CURRENT_LOCATION));
                    return nullptr;
                }
            }
            else if (current.getType() == TokenType::PROTECTED)
            { // 遇到protected关键字
                advance();
                /*if (token.getType() == TokenType::STATIC)
                {
                    advance();
                    isStatic = true;
                }*/
                if (current.getType() == TokenType::FUNC)
                { // 遇到函数申明
                    auto stmt = parse_statement();
                    if (stmt == nullptr)
                    {
                        return nullptr;
                    }
                    c->addMember(AccessLevel::Protected, MemberType::Method, std::move(stmt));
                }
                else if (current.getType() == TokenType::VAR)
                { // 遇到变量申明
                    auto stmt = parse_statement();
                    if (stmt == nullptr)
                    {
                        return nullptr;
                    }
                    c->addMember(AccessLevel::Protected, MemberType::Field, std::move(stmt));
                }
                else
                {
                    em_->emit_invalid_token_after_contorller_for_class(current.getLocation(), "protected", get_code_line_string(CURRENT_LOCATION));
                    return nullptr;
                }
            }
            else
            { // 没有遇到访问修饰关键字，默认为私有
                /*if (token.getType() == TokenType::STATIC)
                {
                    advance();
                    isStatic = true;
                }*/
                if (current.getType() == TokenType::FUNC)
                { // 遇到函数申明
                    auto stmt = parse_statement();
                    if (stmt == nullptr)
                    {
                        return nullptr;
                    }
                    c->addMember(AccessLevel::Private, MemberType::Method, std::move(stmt));
                }
                else if (current.getType() == TokenType::VAR)
                { // 遇到变量申明
                    auto stmt = parse_statement();
                    if (stmt == nullptr)
                    {
                        return nullptr;
                    }
                    c->addMember(AccessLevel::Private, MemberType::Field, std::move(stmt));
                }
                else
                {
                    em_->emit_invalid_class_member_error(current.getLocation(), current.getLiteral(), get_code_line_string(CURRENT_LOCATION));
                    return nullptr;
                }
            }
            // advance();
        }
        if (current.getType() == TokenType::END)
        {
            em_->emit_miss_token_error(current.getLocation(), "}", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过右大括号
        auto t = current;
        if (current.getType() == TokenType::SEMICOLON) // 如果结尾是分号，跳过
            advance();
        return c;
    }
    std::unique_ptr<ThrowStatement> Parser::parse_throw_statement()
    {
        auto stmt = std::make_unique<ThrowStatement>(current);
        advance(); // 跳过 throw 关键字
        auto exception = parse_expression(Priority::LOWEST);
        if (!exception)
        {
            return nullptr;
        }
        stmt->setException(std::move(exception));
        if (currentIs(TokenType::SEMICOLON))
        {
            advance();
        }
        return stmt;
    }
    std::unique_ptr<Statement> Parser::parse_statement()
    {
        switch (current.getType())
        {
        case TokenType::END:
            return nullptr;
        case TokenType::THROW:
            return parse_throw_statement();
        case TokenType::FOR:
            return parse_for_loop();
        case TokenType::TRY:
            return parse_try_catch_statement();
        case TokenType::IMPORT:
            return parse_import_statement();
        case TokenType::VAR: // 解析变量定义
        case TokenType::CONST:
            return parse_declare_statement();
        case TokenType::EXTERN: // 遇到 extern 关键字
            return parse_extern_prefix();
        case TokenType::FUNC:
            return parse_func_statement();
        case TokenType::RETURN:
            return parse_return_statement();
        case TokenType::CLASS:
            return parse_class_statement();
        case TokenType::BREAK:
        case TokenType::CONTINUE:
            return parse_short_statement();
        default:
            return parse_expression_statement();
        }
        return nullptr;
    }
    std::unique_ptr<Statement> Parser::parse_extern_prefix()
    {
        advance(); // 跳过 extern 关键字
        if (match(TokenType::FUNC))
        {
            return parse_func_statement(true); // 返回Statement 子类的std::unique指针
        }
        em_->emit_expect_but_got_error(current.getLocation(), "func", current.getLiteral(), get_code_line_string(CURRENT_LOCATION));
        return nullptr;
    }
    std::unique_ptr<ShortStatement> Parser::parse_short_statement()
    {
        std::unique_ptr<ShortStatement> stmt(new ShortStatement(current));
        advance();
        if (current.getType() == TokenType::SEMICOLON)
            advance();
        return stmt;
    }
    std::unique_ptr<DeclareStatement> Parser::parse_declare_statement()
    {
        bool variable = current.getType() == TokenType::VAR; // 判断是否为常量声明
        std::unique_ptr<DeclareStatement> stmt(new DeclareStatement(current, variable));
        advance(); // 跳过var/const关键字
        Ident *ident = nullptr;
        Expression *exp = nullptr;
        while (1)
        {
            if (!match(TokenType::IDENT))
            {
                em_->emit_ident_missing_error(current.getLocation(), get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            ident = new Ident(current);
            advance();
            if (current.getType() == TokenType::COLON)
            { // 遇到冒号，证明要申明类型
                advance();
                if (!match(TokenType::IDENT))
                {
                    em_->emit_ident_type_missing_error(current.getLocation(), ident->getLiteral(), get_code_line_string(CURRENT_LOCATION));
                    return nullptr;
                }
                auto type = typeManager::find(current.getLiteral());
                if (!type)
                { // 不存在的类型
                    em_->emit_unknown_type_error(current.getLocation(), current.getLiteral(), get_code_line_string(CURRENT_LOCATION));
                    return nullptr;
                }
                ident->setType(type->baseType);
                advance(); // 跳过类型
            }
            // else
            // {
            //     ident->setType("any");
            // }
            if (!variable && !match(TokenType::ASSIGN))
            {
                em_->emit_contrant_not_init_error(current.getLocation(), ident->getLiteral(), get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            if (current.getType() == TokenType::ASSIGN)
            { // 遇到有初始化的申明
                const auto &old_token = current;
                advance(); // 跳过等号
                exp = parse_expression(Priority::LOWEST).release();
                if (!exp)
                {
                    em_->emit_miss_expression_error(old_token.getLocation(), get_code_line_string(old_token.getLocation()));
                    return nullptr;
                }
                // auto type = ident->getType();
                // if (auto i64Exp = dynamic_cast<I64Literal *>(exp))
                // {
                //     // delete exp;
                //     try
                //     {
                //         if (type == "i8")
                //             exp = i64Exp->convertToI8().release();
                //         else if (type == "i16")
                //             exp = i64Exp->convertToI16().release();
                //         else if (type == "i32")
                //             exp = i64Exp->convertToI32().release();
                //         else if (type == "i64")
                //             exp = i64Exp;
                //     }
                //     catch (const std::overflow_error &e)
                //     {
                //         std::cerr << e.what() << '\n';
                //     }
                // }
                // else if (auto f64Exp = dynamic_cast<Float64Literal *>(exp))
                // {
                //     // delete exp;
                //     if (type == "f32")
                //         exp = f64Exp->convertToF32().release();
                //     else if (type == "f64")
                //         exp = f64Exp;
                // }
                // 语法分析阶段，不尝试数字类型转换，直接保存为f64/i64,在语义分析再尝试转换
                stmt->add(ident, exp);
            }
            else
            {
                stmt->add(ident, nullptr);
            }
            if (current.getType() == TokenType::COMMA)
            {
                advance();
            }
            else
            {
                break;
            }
        }
        if (current.getType() == TokenType::SEMICOLON)
        {
            advance();
        }
        return stmt;
    }
    std::unique_ptr<LambdaExpr> Parser::parse_lambda_expression()
    {
        std::unique_ptr<LambdaExpr> exp = std::make_unique<LambdaExpr>();
        advance(); // 跳过 func 关键字
        if (!match(TokenType::LPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "{", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        int error = 0;
        auto params = parse_func_params(error);
        if (error == 1)
        { // 解析函数参数时出错
            return nullptr;
        }
        if (match(TokenType::ARROW))
        {
            advance(); // 跳过 ->
            if (!match(TokenType::IDENT))
            {
                em_->emit_func_return_type_missing_error(current.getLocation(), get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            const std::string &type = current.getLiteral();
            TypeInfo *const &ti = typeManager::find(type);
            if (!ti)
            {
                em_->emit_unknown_type_error(current.getLocation(), type, get_code_line_string(current.getLocation()));
                return nullptr;
            }
            exp->setRetType(ti->baseType);
            advance(); // 跳过返回类型声明
        }
        exp->setParams(params);
        if (!match(TokenType::LBPAREN))
        { // 希望遇到左大括号
            em_->emit_miss_token_error(current.getLocation(), "{", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        // advance(); // 跳过左大括号
        auto body = parse_blockstatement(0);
        if (body == nullptr)
        { // 函数体解析出错
            return nullptr;
        }
        exp->setBody(body.release());
        return exp;
    }
    std::unique_ptr<IfExpression> Parser::parse_if_expression()
    {
        std::unique_ptr<IfExpression> ifstmt = std::make_unique<IfExpression>();
        advance(); // 跳过 if 关键字
        if (!match(TokenType::LPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "(", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance();
        auto condition = parse_expression(Priority::LOWEST);
        if (!condition)
            return nullptr;
        // std::cout<<condition->toString()<<std::endl;
        ifstmt->setCondition(condition.release());
        if (!match(TokenType::RPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), ")", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过右括号
        std::unique_ptr<BlockStatement> body(new BlockStatement());
        if (current.getType() != TokenType::LBPAREN)
        {
            auto consequence = parse_statement();
            if (consequence == nullptr)
                return nullptr;
            body->append(consequence.release());
        }
        else
        {
            body = parse_blockstatement(0);
        }
        ifstmt->setConsequence(body.release());
        if (current.getType() == TokenType::ELSE)
        {
            advance();
            std::unique_ptr<BlockStatement> body_(new BlockStatement());
            if (current.getType() != TokenType::LBPAREN)
            {
                auto al = parse_statement();
                if (al == nullptr)
                    return nullptr;
                body_->append(al.release());
            }
            else
                body_ = parse_blockstatement(0);
            ifstmt->setAlternative(body_.release());
        }
        return ifstmt;
    }

    std::unique_ptr<WhileExpression> Parser::parse_while_expression()
    {
        std::unique_ptr<WhileExpression> we(new WhileExpression());
        advance();
        if (!match(TokenType::LPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "(", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance();
        auto condition = parse_expression(Priority::LOWEST);
        if (!condition)
        {
            return nullptr;
        }
        we->setCondtion(condition.release());
        if (!match(TokenType::RPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), ")", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance();
        if (!match(TokenType::LBPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "{", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        auto body = parse_blockstatement(1);
        if (!body)
            return nullptr;
        we->setBody(body.release());
        return we;
    }

    std::unique_ptr<UntilExpression> Parser::parse_until_expresssion()
    {
        std::unique_ptr<UntilExpression> loop(new UntilExpression());
        advance();
        if (!match(TokenType::LPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "(", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance();
        auto condition = parse_expression(Priority::LOWEST);
        if (!condition)
        {
            return nullptr;
        }
        loop->setCondtion(condition.release());
        if (!match(TokenType::RPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), ")", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance();
        if (!match(TokenType::LBPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "{", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        auto body = parse_blockstatement(1);
        if (!body)
            return nullptr;
        loop->setBody(body.release());
        return loop;
    }
    std::unique_ptr<SwitchExpression> Parser::parse_switch_expression()
    {
        auto expr = std::make_unique<SwitchExpression>();
        advance(); // 跳过switch关键字
        if (!match(TokenType::LPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "(", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过左括号
        auto _ = parse_expression(Priority::LOWEST);
        if (!_)
        {
            return nullptr;
        }
        expr->setExpr(std::move(_));
        if (!match(TokenType::RPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), ")", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过右括号
        if (!match(TokenType::LBPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "{", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过左大括号
        while (current.getType() == TokenType::CASE)
        {
            advance(); // 跳过case关键字
            auto value = parse_expression(Priority::LOWEST);
            if (!value)
            {
                return nullptr;
            }
            if (!match(TokenType::COLON))
            {
                em_->emit_miss_token_error(current.getLocation(), ":", get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            advance(); // 跳过冒号
            auto body = parse_blockstatement(0);
            if (!body)
            {
                return nullptr;
            }
            auto _case = std::make_unique<SwitchCase>();
            if (!_case)
            {
                return nullptr;
            }
            _case->mcase = std::move(value);
            _case->mbody = std::move(body);
            expr->addCase(std::move(_case));
        }
        if (currentIs(TokenType::DEFAULT))
        {              // 遇到default关键字
            advance(); // 跳过 default 关键字
            if (!match(TokenType::COLON))
            {
                em_->emit_miss_token_error(current.getLocation(), ":", get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            advance(); // 跳过冒号
            auto default_body = parse_blockstatement(0);
            if (!default_body)
            {
                return nullptr;
            }
            auto default_case = std::make_unique<DefaultSwitchCase>();
            default_case->mbody = std::move(default_body);
            expr->setDefaultCase(std::move(default_case));
        }
        if (!match(TokenType::RBPAREN))
        {
            em_->emit_miss_token_error(current.getLocation(), "}", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        advance(); // 跳过右大括号
        // if(current.getType() == TokenType::SEMICOLON){
        //     advance();//跳过尾随分号
        // }
        return expr;
    }
    std::unique_ptr<Expression> Parser::parse_prefix_expression()
    {
        auto tt = current.getType();
        switch (tt)
        {
        case TokenType::SWITCH:
            return parse_switch_expression();
        case TokenType::UNTIL:
            return parse_until_expresssion();
        case TokenType::WHILE:
            return parse_while_expression();
        case TokenType::IF:
            return parse_if_expression();
        case TokenType::FUNC:
            return parse_lambda_expression();
        case TokenType::IDENT:
            return parse_ident_expression();
        case TokenType::STRING:
            return parse_string_expression();
        case TokenType::TRUE:
        case TokenType::FALSE:
            return parse_bool_expression();
        case TokenType::INTEAGER:
            return parse_integer_expression();
        case TokenType::NUMBER:
            return parse_float_expression();
        case TokenType::LBRACKET:
            return parse_array_expression();
        case TokenType::THIS:
        {
            auto current_token = current;
            advance();
            return std::make_unique<ThisExpr>(current_token);
        }
        case TokenType::NONE:
        {
            auto current_token = current;
            advance();
            return std::make_unique<NullLiteral>(current_token);
        }
        // 一元运算符
        case TokenType::BANG:
        case TokenType::MINUS:
        case TokenType::LINC:
        case TokenType::LDEC:
        case TokenType::TYPEID:
        {
            Token op = current;
            advance();
            auto right = parse_expression(Priority::PREFIX);
            auto ret = std::make_unique<PrefixExpression>(op);
            ret->setRight(right.release());
            return ret;
        }

        // new关键字
        case TokenType::NEW:
        {
            Token op = current;
            advance();
            auto right = parse_expression(Priority::NEW);
            // std::cout << right->toString() << std::endl;
            auto ret = std::make_unique<NewExpression>(op);
            ret->setRight(right.release());
            return ret;
        }

        // 括号表达式
        case TokenType::LPAREN:
        {
            advance(); // 跳过 '('
            auto expr = parse_expression(Priority::LOWEST);
            if (!match(TokenType::RPAREN))
            {
                // error("缺少右括号 ')'");
                em_->emit_miss_token_error(current.getLocation(), ")", get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            advance(); // 跳过 ')'
            return expr;
        }

        default:
            em_->emit_unexpected_token_error(current.getLocation(), current.getLiteral(), get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
    }
    std::unique_ptr<ImportStatement> Parser::parse_import_statement()
    {
        advance(); // 跳过 import 关键字
        if (!match(TokenType::STRING))
        {
            em_->emit_miss_token_error(current.getLocation(), "[STRING]", get_code_line_string(CURRENT_LOCATION));
            return nullptr;
        }
        auto stmt = std::make_unique<ImportStatement>(current);
        advance(); // 跳过库名
        if (current.getType() == TokenType::SEMICOLON)
        {
            advance();
        }
        return stmt;
    }
    std::unique_ptr<Expression> Parser::parse_infix_expression(std::unique_ptr<Expression> left, Token op)
    {
        Priority precedence = get_precedence();

        // 特殊处理赋值运算符（右结合性）
        if (op.getType() == TokenType::ASSIGN)
        {
            // advance();
            auto right = parse_expression(precedence);
            if (right == nullptr)
                return nullptr;
            auto ret = std::make_unique<InfixExpression>(op);
            ret->setRight(right.release());
            ret->setLeft(left.release());
            return ret;
        }

        else if (op.getType() == TokenType::POINT)
        {
            // advance(); // 跳过点运算符
            if (!match(TokenType::IDENT))
            {
                em_->emit_miss_object_member_error(current.getLocation(), get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            auto right = parse_ident_expression(); // 解析右侧标识符
            auto ret = std::make_unique<InfixExpression>(op);
            ret->setLeft(left.release());
            ret->setRight(right.release());
            return ret;
        }

        // 处理函数调用 ()
        else if (op.getType() == TokenType::LPAREN)
        {
            // auto t=tokens[idx];
            // advance(); // 跳过 '('
            std::vector<std::unique_ptr<Expression>> args;

            if (!match(TokenType::RPAREN))
            {
                do
                {
                    args.push_back(parse_expression(Priority::LOWEST));
                } while (match(TokenType::COMMA) && (advance(), true));
            }

            if (!match(TokenType::RPAREN))
            {
                // error("缺少右括号 ')'");
                em_->emit_miss_token_error(current.getLocation(), ")", get_code_line_string(CURRENT_LOCATION));
                return nullptr;
            }
            advance(); // 跳过 ')'
            auto ret = std::make_unique<CallExpression>();
            ret->setArgs(args);
            ret->setFunc(left.release());
            // std::cout<<ret->toString()<<std::endl;
            return ret;
        }
        else if (op.getType() == TokenType::LBRACKET)
        {
            return parse_index_expression(left.release());
        }
        else
        {
            // advance();
            auto right = parse_expression(precedence);
            if (!right)
                return nullptr;
            auto ret = std::make_unique<InfixExpression>(op);
            ret->setRight(right.release());
            ret->setLeft(left.release());
            return ret;
        }
    }
    std::unique_ptr<ReturnStatement> Parser::parse_return_statement()
    {
        std::unique_ptr<ReturnStatement> retstmt(new ReturnStatement(current));
        advance();
        if (current.getType() == TokenType::SEMICOLON)
        {
            advance();
            return retstmt;
        }
        auto e = parse_expression(Priority::LOWEST);
        retstmt->setExpression(e.release());
        if (current.getType() == TokenType::SEMICOLON)
            advance();
        return retstmt;
    }
    std::unique_ptr<Expression> Parser::parse_expression(Priority priority)
    {
        auto left = parse_prefix_expression();
        if (!left)
            return nullptr;

        // 处理中缀运算符（根据优先级判断是否继续解析）
        while (true)
        {
            // 对于右结合的赋值运算符，使用 >= 判断
            bool should_continue = false;
            if (current.getType() == TokenType::ASSIGN)
            {
                should_continue = priority <= get_precedence();
            }
            else
            {
                should_continue = priority < get_precedence();
            }

            if (!should_continue || current.getType() == TokenType::END)
                break;

            Token op = current;
            advance();
            left = parse_infix_expression(std::move(left), op);
            //  std::cout<<left->toString()<<std::endl;
            if (!left)
                break;
        }

        return left;
    }

    std::unique_ptr<Float64Literal> Parser::parse_float_expression()
    {
        auto ret = std::make_unique<Float64Literal>(current);
        advance();
        return ret;
    }

    std::unique_ptr<I64Literal> Parser::parse_integer_expression()
    {
        auto ret = std::make_unique<I64Literal>(current);
        advance();
        return ret;
    }

    std::unique_ptr<Ident> Parser::parse_ident_expression()
    {
        auto ret = std::make_unique<Ident>(current);
        advance();
        return ret;
    }
    std::unique_ptr<StringLiteral> Parser::parse_string_expression()
    {
        auto ret = std::make_unique<StringLiteral>(current);
        advance();
        return ret;
    }

    std::unique_ptr<BoolLiteral> Parser::parse_bool_expression()
    {
        auto ret = std::make_unique<BoolLiteral>(current);
        advance();
        return ret;
    }
}