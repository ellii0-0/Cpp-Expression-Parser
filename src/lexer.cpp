#include "lexer.h"

std::string symbols = "()=,+-*/^";      // string auxiliar

Token lex_string(std::string &src, std::size_t &i, char &c);
Token lex_number(std::string &src, std::size_t &i, char &c);
Token lex_operator(std::string &src, std::size_t &i, char &c);

std::vector<Token> lexical_analysis(std::string src)
{
        std::vector<Token> tokens;

        std::size_t len = src.size() + 1;

        for (std::size_t i = 0; i < len; i++) {
                char c = src[i];

                if (std::isspace(c))
                        continue;

                // Tokens string (a-z e A-Z)

                if (std::isalpha(c)) {
                        Token token = lex_string(src, i, c);
                        tokens.push_back(token);
                        continue;
                }

                // Tokens numéricos

                if (std::isdigit(c) || c == '.') {
                        Token token = lex_number(src, i, c);
                        tokens.push_back(token);
                        continue;
                }

                // Tokens de apenas um caractere

                Token token = lex_operator(src, i, c);
                tokens.push_back(token);
        }

        return tokens;
}

Token lex_string(std::string &src, std::size_t &i, char &c)
{
        std::size_t len = src.size() + 1;
        std::size_t start = i;

        while ((std::isalnum(c) || c == '_') && i < len) {
                i++;
                char c = src[i];
        }

        std::size_t size = i - start;

        Token token = {
                TokenType::STRING,
                src.substr(start, size),
                0.0,
                start
        };

        return token;
}

Token lex_number(std::string &src, std::size_t &i, char &c)
{
        std::size_t len = src.size() + 1;
        std::size_t start = i;

        while (std::isdigit(c) && i < len) {
                // Lê os primeiros dígitos do número
                i++;
                c = src[i];
        }

        if (c == '.' && i < len) {
                // Lê os dígitos depois do ponto
                i++;
                c = src[i];

                while (std::isdigit(c)) {
                        i++;
                        c = src[i];
                }
        }

        if ((src[i] == 'e' || src[i] == 'E') && i < len) {
                // Lê notação científica
                i++;
                c = src[i];

                if (c == '+' || c == '-') {
                        i++;
                        c = src[i];
                }

                if (!std::isdigit(c))
                        throw LexError("Literal numérico inválido", start);

                while (std::isdigit(c)) {
                        i++;
                        c = src[i];
                }
        }

        if (src.find(c) == std::string::npos || !std::isspace(c) || c != '\0')
                throw LexError("Literal numérico inválido", start);
                        
        std::size_t size = i - start;

        std::string substr = src.substr(start, size);
        double num = std::stod(substr);

        Token token = {
                TokenType::NUMBER,
                std::move(substr),
                num,
                start
        };

        return token;
}

Token lex_operator(std::string &src, std::size_t &i, char &c)
{
        std::size_t start = i;
        TokenType type = TokenType::NONE;

        switch (c) {
        case '\0':
                type = TokenType::END;
                break;
        case '(':
                type = TokenType::LEFT_PAREN;
                break;
        case ')':
                type = TokenType::RIGHT_PAREN;
                break;
        case '=':
                type = TokenType::EQUALS;
                break;
        case ',':
                type = TokenType::COMMA;
                break;
        case '+':
                type = TokenType::PLUS;
                break;
        case '-':
                type = TokenType::MINUS;
                break;
        case '*':
                type = TokenType::TIMES;
                break;
        case '/':
                type = TokenType::DIVISION;
                break;
        case '^':
                type = TokenType::POW;
                break;
        }

        if (type == TokenType::NONE)
                throw LexError("Token inválido", start);

        Token token = {
                type,
                std::string(1, c),
                0.0,
                start
        };

        return token;
}