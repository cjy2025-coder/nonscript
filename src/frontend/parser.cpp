#include "./frontend/parser.h"

namespace ns
{
    void Parser::setLexer(Lexer *lexer)
    {
        mLexer = lexer;
    }
    bool Parser::currentIs(TokenType type) const
    {
        return current().getType() == type;
    }
    Priority Parser::get_precedence() const
    {
        auto tt = current().getType();
        switch (tt)
        {
        case TokenType::ASSIGN:
            return Priority::ASSIGN;
        case TokenType::EQ:
        case TokenType::NEQ:
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
        // bool error = 0;
        while (current().getType() != TokenType::END && current().getType() != TokenType::ILLEGAL)
        {
            auto stmt = parse_statement();
            if (stmt == NULL)
            {
                // error = 1;
                return NULL;
            }
            // return NULL;
            if (typeid(*stmt) == typeid(ShortStatement))
            {
                raiseError(ErrorType::SYNTAX_ERROR, "unexpect " + stmt->toString(), current().getLocation());
                // error = 1;
                return NULL;
            }
            // if (!error)
            program->append(stmt.release());
        }
        if (current().getType() == TokenType::ILLEGAL)
        {
            raiseError(ErrorType::SYNTAX_ERROR, "unexpect token : " + current().getLiteral(), current().getLocation());
            return NULL;
        }
        // if (error)
        //     return NULL;
        return program;
    }
    std::unique_ptr<ForLoop> Parser::parse_for_loop()
    {
        auto loop = std::make_unique<ForLoop>();
        advance(); // 跳过 for 关键字
        if (!match(TokenType::LPAREN))
        {
            return nullptr;
        }
        advance(); // 跳过左括号
        while (current().getType() == TokenType::IDENT)
        {
            auto ident = parse_ident_expression();
            if (!ident)
            {
                return nullptr;
            }
            if (!match(TokenType::ASSIGN))
            {
                return nullptr;
            }
            advance(); // 跳过等号;
            auto rvalue = parse_expression(Priority::LOWEST);
            if (!rvalue)
            {
                return nullptr;
            }
            auto _ = std::make_unique<LoopVariable>();
            _->var = std::move(ident);
            _->value = std::move(rvalue);
            loop->addVariable(std::move(_));
            if (currentIs(TokenType::COMMA))
            {
                advance(); // 跳过逗号
            }
        }
        if (!match(TokenType::SEMICOLON))
        {
            return nullptr;
        }
        advance(); // 跳过分号
        auto condition = parse_expression(Priority::LOWEST);
        if (!condition)
        {
            return nullptr;
        }
        loop->setConditon(std::move(condition));
        if (!match(TokenType::SEMICOLON))
        {
            return nullptr;
        }
        advance(); // 跳过分号
        if (!currentIs(TokenType::RPAREN))
            while (1)
            {
                auto _ = parse_expression(Priority::LOWEST);
                if (!_)
                {
                    return nullptr;
                }
                loop->addAction(std::move(_));
                if (currentIs(TokenType::RPAREN) || currentIs(TokenType::END))
                {
                    break;
                }
                if (currentIs(TokenType::COMMA))
                {
                    advance(); // 跳过逗号
                }
            }
        if (!match(TokenType::RPAREN))
        {
            return nullptr;
        }
        advance(); // 跳过右括号
        auto body = parse_blockstatement(1);
        if (!body)
        {
            return nullptr;
        }
        loop->setBody(std::move(body));
        if (currentIs(TokenType::SEMICOLON))
        {
            advance(); // 跳过尾随分号
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
            return nullptr;
        }
        advance(); // 跳过catch关键字
        if (!match(TokenType::LPAREN))
        {
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
            return NULL;
        }
        advance();
        return ie;
    }
    std::vector<std::unique_ptr<Expression>> Parser::parse_expression_list(TokenType end, int &error)
    {
        std::vector<std::unique_ptr<Expression>> args;
        if (peek().getType() == end)
        {
            advance();
            return {};
        }
        advance();
        auto arg = parse_expression(Priority::LOWEST);
        args.emplace_back(arg.release());
        while (current().getType() == TokenType::COMMA)
        {
            advance();
            args.emplace_back(parse_expression(Priority::LOWEST).release());
        }
        if (!match(end))
        {
            error = 1;
            return {};
        }
        advance();
        return args;
    }

    std::unique_ptr<ArrayLiteral> Parser::parse_array_expression()
    {
        std::unique_ptr<ArrayLiteral> arr(new ArrayLiteral(current()));
        int error = 0;
        auto elems = parse_expression_list(TokenType::RBRACKET, error);
        if (error)
        { // 数组元素解析出错
            return NULL;
        }
        arr->setElems(elems);
        return arr;
    }

    std::vector<std::shared_ptr<Expression>> Parser::parse_func_params(int &error)
    { // 进来时token为左括号
        std::vector<std::shared_ptr<Expression>> params;
        advance(); // 跳过左括号
        if (current().getType() == TokenType::RPAREN)
        {
            advance();
            return {};
        }
        do
        {
            if (!match(TokenType::IDENT))
            {
                error = 1;
                return {};
            }
            auto ident = parse_ident_expression();
            if (current().getType() == TokenType::COLON)
            { // 参数被声明了类型
                advance();
                if (!match(TokenType::IDENT))
                { // 希望冒号后面为参数类型
                    error = 1;
                    return {};
                }
                auto type = parse_ident_expression()->getLiteral();
                ident->setType(type);
            }
            if (current().getType() == TokenType::ASSIGN)
            { // 函数参数有初始值
                auto param = std::make_unique<InfixExpression>(current());
                advance(); // 跳过等号
                auto val = parse_expression(Priority::LOWEST);
                if (!val)
                {
                    raiseError(ErrorType::SYNTAX_ERROR, "expect expression after =", current().getLocation());
                    error = 1;
                    return {};
                }
                param->setLeft(ident.release());
                param->setRight(val.release());
                params.emplace_back(param.release());
                auto it = current();
            }
            else
                params.emplace_back(ident.release());
        } while (current().getType() == TokenType::COMMA && (advance(), true));
        if (!match(TokenType::RPAREN))
        { // 希望遇到右括号以结束参数申明
            error = 1;
            return {};
        }
        advance(); // 跳过右括号
        return params;
    }

    std::unique_ptr<FuncDecl> Parser::parse_func_statement()
    {
        advance(); // 跳过func关键字
        if (!match(TokenType::IDENT))
        { // func关键字后为函数名，应为标识符
            return NULL;
        }
        std::unique_ptr<FuncDecl> func = std::make_unique<FuncDecl>(current());
        advance(); // 跳过标识符
        if (!match(TokenType::LPAREN))
        { // 判断函数名后是否有左括号
            return NULL;
        }
        int error = 0;
        auto params = parse_func_params(error);
        if (error == 1)
        { // 解析函数参数时出错
            return NULL;
        }
        func->setParams(params);
        if (!match(TokenType::LBPAREN))
        { // 希望遇到左大括号
            return NULL;
        }
        // advance(); // 跳过左大括号
        auto body = parse_blockstatement(0);
        if (body == NULL)
        { // 函数体解析出错
            return NULL;
        }
        func->setBody(body.release());
        // auto it = current();
        return func;
    }

    std::unique_ptr<BlockStatement> Parser::parse_blockstatement(int loop)
    {
        if(!match(TokenType::LBPAREN)){
            return nullptr;
        }
        std::unique_ptr<BlockStatement> bstmt(new BlockStatement(current()));
        advance();
        while (current().getType() != TokenType::RBPAREN && current().getType() != TokenType::END)
        {
            auto stmt = parse_statement();
            if (stmt == NULL)
                return NULL;
            if (typeid(*stmt) == typeid(ShortStatement) && !loop) // 除了循环语句外，其它语句不能出现break,continue
            {
                raiseError(ErrorType::SYNTAX_ERROR, "unexpect " + stmt->toString(), current().getLocation());
                return NULL;
            }
            if (stmt != NULL)
                bstmt->append(stmt.release());
            // advance();
        }
        if (!match(TokenType::RBPAREN))
        {
            return NULL;
        }
        advance(); // 跳过右大括号
        return bstmt;
    }

    std::unique_ptr<ExpressionStatement> Parser::parse_expression_statement()
    {
        std::unique_ptr<ExpressionStatement> estmt(new ExpressionStatement(current()));
        auto e = parse_expression(Priority::LOWEST);
        if (e == NULL)
        {
            // if(peek().getType() != TokenType::END) advance();
            return NULL;
        }
        estmt->setExpression(e.release());
        auto it = idx;
        if (current().getType() == TokenType::SEMICOLON)
            advance();
        return estmt;
    }

    std::unique_ptr<ClassLiteral> Parser::parse_class_statement()
    {
        advance();                                                    // 跳过class关键字
        std::unique_ptr<ClassLiteral> c(new ClassLiteral(current())); // 将类名传入
        advance();
        if (current().getType() == TokenType::EXTENDS)
        {                                       // 如果有父类
            advance();                          // 跳过extends关键字
            auto id = parse_ident_expression(); // 解析父类名
            if (id == NULL)
            {
                raiseError(ErrorType::SYNTAX_ERROR, "expect a class name after :extends", current().getLocation());
                return NULL;
            }
            c->addBaseClass(std::move(id)); // 设置父类
            while (peek().getType() == TokenType::COMMA)
            { // 如果有多个父类
                advance();
                advance(); // 跳过逗号
                auto id = parse_ident_expression();
                if (id == NULL)
                {
                    raiseError(ErrorType::SYNTAX_ERROR, "expect a class name after `,`", current().getLocation());
                    return NULL;
                }
                c->addBaseClass(std::move(id));
                // advance();//跳过父类名
            }
        }
        if (!match(TokenType::LBPAREN)) // 如果不是左大括号，证明申明有误
        {
            return NULL;
        }
        advance();                                                                                 // 跳过左大括号
        while (current().getType() != TokenType::RBPAREN && current().getType() != TokenType::END) // 解析类成员知道结束或者源文件到头
        {
            if (current().getType() == TokenType::PRIVATE)
            { // 遇到private关键字
                // bool isStatic = false;
                advance();
                /*if (token.getType() == TokenType::STATIC)
                {
                    advance();
                    isStatic = true;
                }*/
                if (current().getType() == TokenType::FUNC)
                { // 遇到函数申明
                    auto stmt = parse_statement();
                    if (stmt == NULL)
                    {
                        return NULL;
                    }
                    c->addMember(AccessLevel::Private, MemberType::Method, std::move(stmt));
                }
                else if (current().getType() == TokenType::VAR)
                { // 遇到变量申明
                    auto stmt = parse_statement();
                    if (stmt == NULL)
                    {
                        return NULL;
                    }
                    c->addMember(AccessLevel::Private, MemberType::Field, std::move(stmt));
                }
                else
                {
                    raiseError(ErrorType::SYNTAX_ERROR, "expect a function or variable after keyword `private`", current().getLocation());
                    return NULL;
                }
            }
            else if (current().getType() == TokenType::PUBLIC)
            { // 遇到public关键字
                advance();
                /*if (token.getType() == TokenType::STATIC)
                {
                    advance();
                    isStatic = true;
                }*/
                if (current().getType() == TokenType::FUNC)
                { // 遇到函数申明
                    auto stmt = parse_statement();
                    if (stmt == NULL)
                    {
                        return NULL;
                    }
                    c->addMember(AccessLevel::Public, MemberType::Method, std::move(stmt));
                }
                else if (current().getType() == TokenType::VAR)
                { // 遇到变量申明
                    auto stmt = parse_statement();
                    if (stmt == NULL)
                    {
                        return NULL;
                    }
                    c->addMember(AccessLevel::Public, MemberType::Field, std::move(stmt));
                }
                else
                {
                    raiseError(ErrorType::SYNTAX_ERROR, "expect a function or variable after keyword `public`", current().getLocation());
                    return NULL;
                }
            }
            else if (current().getType() == TokenType::PROTECTED)
            { // 遇到protected关键字
                advance();
                /*if (token.getType() == TokenType::STATIC)
                {
                    advance();
                    isStatic = true;
                }*/
                if (current().getType() == TokenType::FUNC)
                { // 遇到函数申明
                    auto stmt = parse_statement();
                    if (stmt == NULL)
                    {
                        return NULL;
                    }
                    c->addMember(AccessLevel::Protected, MemberType::Method, std::move(stmt));
                }
                else if (current().getType() == TokenType::VAR)
                { // 遇到变量申明
                    auto stmt = parse_statement();
                    if (stmt == NULL)
                    {
                        return NULL;
                    }
                    c->addMember(AccessLevel::Protected, MemberType::Field, std::move(stmt));
                }
                else
                {
                    raiseError(ErrorType::SYNTAX_ERROR, "expect a function or variable after keyword `protected`", current().getLocation());
                    return NULL;
                }
            }
            else
            { // 没有遇到访问修饰关键字，默认为私有
                /*if (token.getType() == TokenType::STATIC)
                {
                    advance();
                    isStatic = true;
                }*/
                if (current().getType() == TokenType::FUNC)
                { // 遇到函数申明
                    auto stmt = parse_statement();
                    if (stmt == NULL)
                    {
                        return NULL;
                    }
                    c->addMember(AccessLevel::Private, MemberType::Method, std::move(stmt));
                }
                else if (current().getType() == TokenType::VAR)
                { // 遇到变量申明
                    auto stmt = parse_statement();
                    if (stmt == NULL)
                    {
                        return NULL;
                    }
                    c->addMember(AccessLevel::Private, MemberType::Field, std::move(stmt));
                }
                else
                {
                    raiseError(ErrorType::SYNTAX_ERROR, "expect a function or variable for a class member", current().getLocation());
                    return NULL;
                }
            }
            // advance();
        }
        if (current().getType() == TokenType::END)
        {
            raiseError(ErrorType::SYNTAX_ERROR, "syntax error,expect `}` instead of `EOF`", current().getLocation());
            return NULL;
        }
        advance(); // 跳过右大括号
        auto t = current();
        if (current().getType() == TokenType::SEMICOLON) // 如果结尾是分号，跳过
            advance();
        return c;
    }
    std::unique_ptr<ThrowStatement> Parser::parse_throw_statement(){
        auto stmt=std::make_unique<ThrowStatement>(current());
        advance();//跳过 throw 关键字
        auto exception=parse_expression(Priority::LOWEST);
        if(!exception){
            return nullptr;
        }
        stmt->setException(std::move(exception));
        if(currentIs(TokenType::SEMICOLON)){
            advance();
        }
        return stmt;
    }
    std::unique_ptr<Statement> Parser::parse_statement()
    {
        switch (current().getType())
        {
        case TokenType::END:
            return NULL;
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
        return NULL;
    }
    std::unique_ptr<ShortStatement> Parser::parse_short_statement()
    {
        std::unique_ptr<ShortStatement> stmt(new ShortStatement(current()));
        advance();
        if (current().getType() == TokenType::SEMICOLON)
            advance();
        return stmt;
    }
    std::unique_ptr<DeclareStatement> Parser::parse_declare_statement()
    {
        bool variable = current().getType() == TokenType::VAR; // 判断是否为常量声明
        std::unique_ptr<DeclareStatement> stmt(new DeclareStatement(current(), variable));
        advance(); // 跳过var/const关键字
        Ident *ident = NULL;
        Expression *exp = NULL;
        while (1)
        {
            if (!match(TokenType::IDENT))
                return NULL; // 不是标识符情况下报错
            ident = new Ident(current());
            advance();
            if (current().getType() == TokenType::COLON)
            { // 遇到冒号，证明要申明类型
                advance();
                if (!match(TokenType::IDENT))
                    return NULL;
                ident->setType(current().getLiteral());
                advance(); // 跳过类型
            }
            else
            {
                ident->setType("any");
            }
            if (!variable && !match(TokenType::ASSIGN))
                return NULL; // 常量必须被初始化
            if (current().getType() == TokenType::ASSIGN)
            {              // 遇到有初始化的申明
                advance(); // 跳过等号
                exp = parse_expression(Priority::LOWEST).release();
                if (!exp)
                {
                    raiseError(ErrorType::SYNTAX_ERROR, "expect expression after =", current().getLocation());
                    return NULL;
                }
                auto type = ident->getType();
                if (auto i64Exp = dynamic_cast<I64Literal *>(exp))
                {
                    // delete exp;
                    try
                    {
                        if (type == "int8")
                            exp = i64Exp->convertToI8().release();
                        else if (type == "int16")
                            exp = i64Exp->convertToI16().release();
                        else if (type == "int32")
                            exp = i64Exp->convertToI32().release();
                        else if (type == "int64")
                            exp = i64Exp;
                    }
                    catch (const std::overflow_error &e)
                    {
                        std::cerr << e.what() << '\n';
                    }
                }
                else if (auto f64Exp = dynamic_cast<Float64Literal *>(exp))
                {
                    // delete exp;
                    if (type == "f32")
                        exp = f64Exp->convertToF32().release();
                    else if (type == "f64")
                        exp = f64Exp;
                }
                stmt->add(ident, exp);
            }
            else
            {
                stmt->add(ident, NULL);
            }
            if (current().getType() == TokenType::COMMA)
            {
                advance();
            }
            else
            {
                break;
            }
        }
        if (current().getType() == TokenType::SEMICOLON)
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
            return NULL;
        }
        int error = 0;
        auto params = parse_func_params(error);
        if (error == 1)
        { // 解析函数参数时出错
            return NULL;
        }
        exp->setParams(params);
        if (!match(TokenType::LBPAREN))
        { // 希望遇到左大括号
            return NULL;
        }
        // advance(); // 跳过左大括号
        auto body = parse_blockstatement(0);
        if (body == NULL)
        { // 函数体解析出错
            return NULL;
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
            return NULL;
        }
        advance();
        auto condition = parse_expression(Priority::LOWEST);
        if (!condition)
            return NULL;
        // std::cout<<condition->toString()<<std::endl;
        ifstmt->setCondition(condition.release());
        if (!match(TokenType::RPAREN))
        {
            return NULL;
        }
        advance(); // 跳过右括号
        std::unique_ptr<BlockStatement> body(new BlockStatement());
        if (current().getType() != TokenType::LBPAREN)
        {
            auto consequence = parse_statement();
            if (consequence == NULL)
                return NULL;
            body->append(consequence.release());
        }
        else
        {
            body = parse_blockstatement(0);
        }
        ifstmt->setConsequence(body.release());
        if (current().getType() == TokenType::ELSE)
        {
            advance();
            std::unique_ptr<BlockStatement> body_(new BlockStatement());
            if (current().getType() != TokenType::LBPAREN)
            {
                auto al = parse_statement();
                if (al == NULL)
                    return NULL;
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
            return NULL;
        advance();
        auto condition = parse_expression(Priority::LOWEST);
        if (!condition)
        {
            return NULL;
        }
        we->setCondtion(condition.release());
        if (!match(TokenType::RPAREN))
            return NULL;
        advance();
        if (!match(TokenType::LBPAREN))
            return NULL;
        auto body = parse_blockstatement(1);
        if (!body)
            return NULL;
        we->setBody(body.release());
        return we;
    }

    std::unique_ptr<UntilExpression> Parser::parse_until_expresssion()
    {
        std::unique_ptr<UntilExpression> loop(new UntilExpression());
        advance();
        if (!match(TokenType::LPAREN))
            return NULL;
        advance();
        auto condition = parse_expression(Priority::LOWEST);
        if (!condition)
        {
            return NULL;
        }
        loop->setCondtion(condition.release());
        if (!match(TokenType::RPAREN))
            return NULL;
        advance();
        if (!match(TokenType::LBPAREN))
            return NULL;
        auto body = parse_blockstatement(1);
        if (!body)
            return NULL;
        loop->setBody(body.release());
        return loop;
    }
    std::unique_ptr<SwitchExpression> Parser::parse_switch_expression()
    {
        auto expr = std::make_unique<SwitchExpression>();
        advance(); // 跳过switch关键字
        if (!match(TokenType::LPAREN))
        {
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
            return nullptr;
        }
        advance(); // 跳过右括号
        if (!match(TokenType::LBPAREN))
        {
            return nullptr;
        }
        advance(); // 跳过左大括号
        while (current().getType() == TokenType::CASE)
        {
            advance(); // 跳过case关键字
            auto value = parse_expression(Priority::LOWEST);
            if (!value)
            {
                return nullptr;
            }
            if (!match(TokenType::COLON))
            {
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
        if(currentIs(TokenType::DEFAULT)){//遇到default关键字
            advance();//跳过 default 关键字
            if(!match(TokenType::COLON)){
                return nullptr;
            }
            advance();//跳过冒号
            auto default_body=parse_blockstatement(0);
            if(!default_body){
                return nullptr;
            }
            auto default_case=std::make_unique<DefaultSwitchCase>();
            default_case->mbody=std::move(default_body);
            expr->setDefaultCase(std::move(default_case));
        }
        if (!match(TokenType::RBPAREN))
        {
            return nullptr;
        }
        advance(); // 跳过右大括号
        // if(current().getType() == TokenType::SEMICOLON){
        //     advance();//跳过尾随分号
        // }
        return expr;
    }
    std::unique_ptr<Expression> Parser::parse_prefix_expression()
    {
        auto tt = current().getType();
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
        case TokenType::NONE:
        {
            advance();
            return std::make_unique<NullLiteral>();
        }
        // 一元运算符
        case TokenType::BANG:
        case TokenType::MINUS:
        case TokenType::LINC:
        case TokenType::LDEC:
        {
            Token op = current();
            advance();
            auto right = parse_expression(Priority::PREFIX);
            auto ret = std::make_unique<PrefixExpression>(op);
            ret->setRight(right.release());
            return ret;
        }

        // new关键字
        case TokenType::NEW:
        {
            Token op = current();
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
                return nullptr;
            }
            advance(); // 跳过 ')'
            return expr;
        }

        default:
            raiseError(ErrorType::SYNTAX_ERROR, "unexpect expression : " + current().getLiteral(), current().getLocation());
            return nullptr;
        }
    }
    std::unique_ptr<ImportStatement> Parser::parse_import_statement()
    {
        advance(); // 跳过 import 关键字
        if (!match(TokenType::STRING))
        {
            return nullptr;
        }
        auto stmt = std::make_unique<ImportStatement>(current());
        advance(); // 跳过库名
        if (current().getType() == TokenType::SEMICOLON)
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
                raiseError(ErrorType::SYNTAX_ERROR, "expect identifier after '.'", current().getLocation());
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
        std::unique_ptr<ReturnStatement> retstmt(new ReturnStatement(current()));
        advance();
        if (current().getType() == TokenType::SEMICOLON)
        {
            advance();
            return retstmt;
        }
        auto e = parse_expression(Priority::LOWEST);
        retstmt->setExpression(e.release());
        if (current().getType() == TokenType::SEMICOLON)
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
            if (current().getType() == TokenType::ASSIGN)
            {
                should_continue = priority <= get_precedence();
            }
            else
            {
                should_continue = priority < get_precedence();
            }

            if (!should_continue || current().getType() == TokenType::END)
                break;

            Token op = current();
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
        auto ret = std::make_unique<Float64Literal>(current());
        advance();
        return ret;
    }

    std::unique_ptr<I64Literal> Parser::parse_integer_expression()
    {
        auto ret = std::make_unique<I64Literal>(current());
        advance();
        return ret;
    }

    std::unique_ptr<Ident> Parser::parse_ident_expression()
    {
        auto ret = std::make_unique<Ident>(current());
        advance();
        return ret;
    }
    std::unique_ptr<StringLiteral> Parser::parse_string_expression()
    {
        auto ret = std::make_unique<StringLiteral>(current());
        advance();
        return ret;
    }

    std::unique_ptr<BoolLiteral> Parser::parse_bool_expression()
    {
        auto ret = std::make_unique<BoolLiteral>(current());
        advance();
        return ret;
    }
}