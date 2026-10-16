#include "lexer.h"

#include <cctype>
#include <string>
#include <utility>
#include <vector>

const std::string symbols = "()=,+-*/^";      // string auxiliar

static Token lex_string(std::string &src, std::size_t &i, char &c);
static Token lex_number(std::string &src, std::size_t &i, char &c);
static Token lex_operator(std::size_t &i, char &c);

std::vector<Token> lexical_analysis(std::string src)
{
        std::vector<Token> tokens;

        std::size_t len = src.size() + 1;

        for (std::size_t i = 0; i < len; i++) {
                char c = src[i];

                if (std::isspace(c))
                        continue;

                // Tokens string (a-z e A-Z)

                if (std::isalpha(c) || c == '_') {
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

                Token token = lex_operator(i, c);
                tokens.push_back(token);
        }

        return tokens;
}

static Token lex_string(std::string &src, std::size_t &i, char &c)
{
        std::size_t len = src.size() + 1;
        std::size_t start = i;

        while ((std::isalnum(c) || c == '_') && i < len) {
                i++;
                c = src[i];
        }

        std::size_t size = i - start;

        Token token = {
                TokenType::STRING,
                src.substr(start, size),
                0.0,
                start
        };

        i--;

        return token;
}

static Token lex_number(std::string &src, std::size_t &i, char &c)
{
        std::size_t len = src.size() + 1;
        std::size_t start = i;
        bool has_digits = false;

        while (std::isdigit(c) && i < len) {
                // Lê os primeiros dígitos do número
                has_digits = true;
                i++;
                c = src[i];
        }

        if (c == '.' && i < len) {
                // Lê os dígitos depois do ponto
                i++;
                c = src[i];

                while (std::isdigit(c)) {
                        has_digits = true;
                        i++;
                        c = src[i];
                }
        }

        if (!has_digits)
                throw LexError("Literal numérico inválido", start);

        if ((src[i] == 'e' || src[i] == 'E') && i < len) {
                // Lê notação científica
                i++;
                c = src[i];

                if (c == '+' || c == '-') {
                        i++;
                        c = src[i];
                }

                if (!std::isdigit(c))
                        throw LexError("Expoente inválido no literal numérico", start);

                while (std::isdigit(c)) {
                        i++;
                        c = src[i];
                }
        }

        if (!std::isspace(c) && c != '\0' && symbols.find(c) == std::string::npos)
                throw LexError("Literal numérico inválido", start);

        std::size_t size = i - start;

        std::string substr = src.substr(start, size);
        double num;

        try {
                std::size_t converted = 0;
                num = std::stod(substr, &converted);

                if (converted != substr.size())
                        throw LexError("Literal numérico inválido", start);
        }
        catch (const std::invalid_argument &) {
                throw LexError("Literal numérico inválido", start);
        }
        catch (const std::out_of_range &) {
                throw LexError("Literal numérico fora do intervalo de double", start);
        }

        Token token = {
                TokenType::NUMBER,
                std::move(substr),
                num,
                start
        };

        i--;

        return token;
}

static Token lex_operator(std::size_t &i, char &c)
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
                throw LexError("Caractere inválido", start);

        Token token = {
                type,
                std::string(1, c),
                0.0,
                start
        };

        return token;
}