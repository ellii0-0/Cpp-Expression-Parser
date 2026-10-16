#include "lexer.h"

#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace {

const std::string SYMBOLS = "()=,+-*/^";

bool is_digit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool is_alpha(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0;
}

bool is_space(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

bool is_identifier_start(char c) {
    return is_alpha(c) || c == '_';
}

bool is_identifier_part(char c) {
    return is_identifier_start(c) || is_digit(c);
}

Token lex_string(const std::string &src, std::size_t &pos) {
    const std::size_t start = pos;

    while (pos < src.size() && is_identifier_part(src[pos]))
        ++pos;

    return {
        TokenType::STRING,
        src.substr(start, pos - start),
        0.0,
        start
    };
}

Token lex_number(const std::string &src, std::size_t &pos) {
    const std::size_t start = pos;
    bool has_digits = false;

    // Parte inteira.
    while (pos < src.size() && is_digit(src[pos])) {
        has_digits = true;
        ++pos;
    }

    // Parte decimal.
    if (pos < src.size() && src[pos] == '.') {
        ++pos;

        while (pos < src.size() && is_digit(src[pos])) {
            has_digits = true;
            ++pos;
        }
    }

    // Aceita ".5" e "5.", mas não aceita apenas ".".
    if (!has_digits)
        throw LexError("Literal numérico inválido", start);

    //  notação científica.
    if (pos < src.size() &&
        (src[pos] == 'e' || src[pos] == 'E')) {
        ++pos;

        if (pos < src.size() &&
            (src[pos] == '+' || src[pos] == '-')) {
            ++pos;
        }

        // Depois de "e", "e+" ou "e-", deve haver um dígito.
        if (pos >= src.size() || !is_digit(src[pos])) {
            throw LexError(
                "Expoente inválido no literal numérico",
                start
            );
        }

        while (pos < src.size() && is_digit(src[pos]))
            ++pos;
    }

    // Após um número, exige espaço, símbolo válido ou fim da entrada.
    if (pos < src.size() &&
        !is_space(src[pos]) &&
        SYMBOLS.find(src[pos]) == std::string::npos) {
        throw LexError("Literal numérico inválido", start);
    }

    std::string text = src.substr(start, pos - start);
    double value;

    try {
        std::size_t converted = 0;
        value = std::stod(text, &converted);

        if (converted != text.size())
            throw LexError("Literal numérico inválido", start);
    }
    catch (const std::invalid_argument &) {
        throw LexError("Literal numérico inválido", start);
    }
    catch (const std::out_of_range &) {
        throw LexError(
            "Literal numérico fora do intervalo de double",
            start
        );
    }

    return {
        TokenType::NUMBER,
        std::move(text),
        value,
        start
    };
}

Token lex_operator(const std::string &src, std::size_t &pos) {
    const std::size_t start = pos;
    const char c = src[pos];
    TokenType type;

    switch (c) {
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

    default:
        throw LexError("Caractere inválido", start);
    }

    ++pos;

    return {
        type,
        std::string(1, c),
        0.0,
        start
    };
}

} // namespace

std::vector<Token> lexical_analysis(std::string src) {
    std::vector<Token> tokens;
    std::size_t pos = 0;

    while (pos < src.size()) {
        const char c = src[pos];

        if (is_space(c)) {
            ++pos;
            continue;
        }

        if (is_identifier_start(c)) {
            tokens.push_back(lex_string(src, pos));
            continue;
        }

        if (is_digit(c) || c == '.') {
            tokens.push_back(lex_number(src, pos));
            continue;
        }

        tokens.push_back(lex_operator(src, pos));
    }

    // O fim da entrada é adicionado explicitamente.
    tokens.push_back({
        TokenType::END,
        "",
        0.0,
        src.size()
    });

    return tokens;
}