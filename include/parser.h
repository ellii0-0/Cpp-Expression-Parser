#ifndef PARSER_H
#define PARSER_H

#include <cstddef>
#include <string>
#include <stdexcept>

#include "ast.h"

// erro do parser
class ParseError : public std::runtime_error {
private:
        std::size_t position;   // posição do token
public:
        ParseError(const std::string &msg, std::size_t position)
                : std::runtime_error(msg), position(position) {}
        std::size_t pos() const { return position; }
};

ExprPtr parse_text(std::string src);

#endif