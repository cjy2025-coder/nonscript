#include "frontend/token.h"

namespace ns
{
    Token::Token(sourceLocation _location, std::string _literal, TokenType _type)
    {
        location=_location;
        literal=_literal;
        type=_type;
    }

    sourceLocation Token::getLocation() const
    {
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
    std::string Token::toString() const
    {
        return "( " + names.find(type)->second + " ," + literal + " )";;
    }

    //  TokenType lookup(const std::string &literal)
    //  {
    //      if (literal == "or")
    //          return TokenType::OR;
    //      else if (literal == "and")
    //          return TokenType::AND;
    //      auto it = keywords.find(literal);
    //      if (it != keywords.end())
    //          return it->second;
    //      return TokenType::IDENT;
    //  }
}
