#include "frontend/token.h"

namespace ns
{
    Token::Token(sourceLocation _location, const std::string & _literal, TokenType _type){
        location=_location;
        literal=_literal;
        type=_type;
    }

    sourceLocation Token::getLocation() const{
        return location;
    }
    std::string Token::getLiteral() const
    {
        return literal;
    }
    TokenType Token::getType() const
    {
        return type;
    }
    std::string  Token::toString() const
    {
        const auto &token_type = names.find(type)->second;
        return "( " + token_type + " ," + literal + " )";;
    }
} 
