#include "ast.h"
#include "parser.h"
#include "lexer.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

struct OpInfo {
    char op;
    int precedence;
    bool right_assoc;
};

constexpr int UNARY_PRECEDENCE = 3;

const Token &peek_token(
    const std::vector<Token> &tokens,
    std::size_t pos
) {
    if (pos >= tokens.size()) {
        std::size_t error_pos = tokens.empty()
            ? 0
            : tokens.back().pos;

        throw ParseError("Fim inesperado da entrada", error_pos);
    }

    return tokens[pos];
}

const Token &consume_token(
    const std::vector<Token> &tokens,
    std::size_t &pos,
    TokenType expected = TokenType::NONE,
    const std::string &msg = "Token inesperado"
) {
    const Token &tok = peek_token(tokens, pos);

    if (expected != TokenType::NONE && tok.type != expected)
        throw ParseError(msg, tok.pos);

    ++pos;
    return tok;
}

bool binop_info(TokenType type, OpInfo &info) {
    switch (type) {
    case TokenType::PLUS:
        info = {'+', 1, false};
        return true;

    case TokenType::MINUS:
        info = {'-', 1, false};
        return true;

    case TokenType::TIMES:
        info = {'*', 2, false};
        return true;

    case TokenType::DIVISION:
        info = {'/', 2, false};
        return true;

    case TokenType::POW:
        info = {'^', 4, true};
        return true;

    default:
        return false;
    }
}

ExprPtr parse_expr(
    const std::vector<Token> &tokens,
    std::size_t &pos,
    int min_precedence
);

ExprPtr parse_unary(
    const std::vector<Token> &tokens,
    std::size_t &pos
);

ExprPtr parse_primary(
    const std::vector<Token> &tokens,
    std::size_t &pos
) {
    const Token &tok = consume_token(tokens, pos);

    switch (tok.type) {
    case TokenType::NUMBER:
        return std::make_unique<Number>(tok.num);

    case TokenType::STRING: {
        std::string name = tok.text;

        // Apenas consulta: o token seguinte ainda não é consumido.
        if (peek_token(tokens, pos).type != TokenType::LEFT_PAREN)
            return std::make_unique<Variable>(std::move(name));

        consume_token(tokens, pos, TokenType::LEFT_PAREN);

        Args args;

        // Permite chamadas sem argumentos, como f().
        if (peek_token(tokens, pos).type != TokenType::RIGHT_PAREN) {
            while (true) {
                args.push_back(parse_expr(tokens, pos, 1));

                if (peek_token(tokens, pos).type != TokenType::COMMA)
                    break;

                consume_token(tokens, pos, TokenType::COMMA);
            }
        }

        // O ')' é consumido mesmo quando não há argumentos.
        consume_token(
            tokens,
            pos,
            TokenType::RIGHT_PAREN,
            "Esperado ')' após os argumentos da função"
        );

        return std::make_unique<Call>(
            std::move(name),
            std::move(args)
        );
    }

    case TokenType::LEFT_PAREN: {
        ExprPtr expr = parse_expr(tokens, pos, 1);

        consume_token(
            tokens,
            pos,
            TokenType::RIGHT_PAREN,
            "Parêntese direito ')' faltante"
        );

        return expr;
    }

    default:
        throw ParseError("Esperada uma expressão", tok.pos);
    }
}

ExprPtr parse_unary(
    const std::vector<Token> &tokens,
    std::size_t &pos
) {
    const Token &tok = peek_token(tokens, pos);

    if (tok.type != TokenType::PLUS &&
        tok.type != TokenType::MINUS) {
        return parse_primary(tokens, pos);
    }

    const bool negative = tok.type == TokenType::MINUS;
    consume_token(tokens, pos);

    ExprPtr rhs = parse_expr(tokens, pos, UNARY_PRECEDENCE);

    if (negative)
        return std::make_unique<UnOp>('-', std::move(rhs));

    return rhs;
}

ExprPtr parse_expr(
    const std::vector<Token> &tokens,
    std::size_t &pos,
    int min_precedence
) {
    ExprPtr lhs = parse_unary(tokens, pos);

    while (true) {
        const Token &tok = peek_token(tokens, pos);
        OpInfo info{};

        if (!binop_info(tok.type, info) ||
            info.precedence < min_precedence) {
            break;
        }

        consume_token(tokens, pos);

        const int next_precedence = info.right_assoc
            ? info.precedence
            : info.precedence + 1;

        ExprPtr rhs = parse_expr(tokens, pos, next_precedence);

        lhs = std::make_unique<BinOp>(
            info.op,
            std::move(lhs),
            std::move(rhs)
        );
    }

    return lhs;
}

bool matches_funcdef(const std::vector<Token> &tokens) {
    // Reconhece:
    // nome(param1, param2, ...) =
    // ou nome() =
    //
    // Não modifica os tokens nem consome a entrada.

    std::size_t i = 0;

    auto matches = [&](TokenType type) {
        return i < tokens.size() && tokens[i].type == type;
    };

    if (!matches(TokenType::STRING))
        return false;
    ++i;

    if (!matches(TokenType::LEFT_PAREN))
        return false;
    ++i;

    if (!matches(TokenType::RIGHT_PAREN)) {
        while (true) {
            if (!matches(TokenType::STRING))
                return false;
            ++i;

            if (!matches(TokenType::COMMA))
                break;

            ++i;
        }
    }

    if (!matches(TokenType::RIGHT_PAREN))
        return false;
    ++i;

    return matches(TokenType::EQUALS);
}

std::unique_ptr<FuncDef> parse_funcdef(
    const std::vector<Token> &tokens,
    std::size_t &pos
) {
    std::string name = consume_token(
        tokens,
        pos,
        TokenType::STRING,
        "Esperado nome da função"
    ).text;

    consume_token(
        tokens,
        pos,
        TokenType::LEFT_PAREN,
        "Esperado '(' após o nome da função"
    );

    std::vector<std::string> params;

    if (peek_token(tokens, pos).type != TokenType::RIGHT_PAREN) {
        while (true) {
            const Token &param = consume_token(
                tokens,
                pos,
                TokenType::STRING,
                "Esperado nome de parâmetro"
            );

            if (std::find(params.begin(), params.end(), param.text)
                != params.end()) {
                throw ParseError(
                    "Parâmetro repetido: " + param.text,
                    param.pos
                );
            }

            params.push_back(param.text);

            if (peek_token(tokens, pos).type != TokenType::COMMA)
                break;

            consume_token(tokens, pos, TokenType::COMMA);
        }
    }

    consume_token(
        tokens,
        pos,
        TokenType::RIGHT_PAREN,
        "Esperado ')' após os parâmetros"
    );

    consume_token(
        tokens,
        pos,
        TokenType::EQUALS,
        "Esperado '=' na definição da função"
    );

    if (peek_token(tokens, pos).type == TokenType::END) {
        throw ParseError(
            "Corpo da função faltante",
            peek_token(tokens, pos).pos
        );
    }

    ExprPtr body = parse_expr(tokens, pos, 1);

    return std::make_unique<FuncDef>(
        std::move(name),
        std::move(params),
        std::move(body)
    );
}

} // namespace

ExprPtr parse_text(std::string src) {
    const std::vector<Token> tokens = lexical_analysis(std::move(src));

    if (tokens.empty())
        throw ParseError("Vetor de tokens vazio", 0);

    if (tokens.back().type != TokenType::END) {
        throw ParseError(
            "Token de fim da entrada ausente",
            tokens.back().pos
        );
    }

    std::size_t pos = 0;
    ExprPtr expr;

    if (matches_funcdef(tokens))
        expr = parse_funcdef(tokens, pos);
    else
        expr = parse_expr(tokens, pos, 1);

    // Impede aceitar apenas uma parte da entrada.
    consume_token(
        tokens,
        pos,
        TokenType::END,
        "Token inesperado após o fim da expressão"
    );

    if (pos != tokens.size()) {
        throw ParseError(
            "Há tokens após o marcador de fim da entrada",
            tokens[pos].pos
        );
    }

    return expr;
}