#ifndef LEXER_H
#define LEXER_H

#include <stdexcept>
#include <string>
#include <memory>
#include <vector>

// Tipos aceitos de token
enum class TokenType {
        NONE,
        END,
        LEFT_PAREN,             // (
        RIGHT_PAREN,            // )
        EQUALS,                 // =
        COMMA,                  // ,
        NUMBER,                 // número de ponto flutuante
        STRING,                 // string alfabética a-z
        PLUS,                   // +
        MINUS,                  // -
        TIMES,                  // *
        DIVISION,               // /
        POW                     // ^
};

// Tokens produzidos pelo lexer
struct Token {
public:
        TokenType type;         // Tipo do token
        std::string text;       // String do token
        double num;             // Valor do token (apenas para tipo numérico)
        std::size_t pos;        // Posição na string do token
};

// Realiza a análise léxica de uma string
std::vector<Token> lexical_analysis(std::string src);

#endif