#include "ast.h"
#include "parser.h"
#include "lexer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

static Token &consume_token(std::vector<Token> &tokens, int &pos, TokenType type=TokenType::NONE, std::string msg="");
static Token &peek_token(std::vector<Token> &tokens, int pos);

static bool matches_funcdef(std::vector<Token> &tokens);

static std::unique_ptr<FuncDef> parse_funcdef(std::vector<Token> &tokens, int &pos);
static std::unique_ptr<Expr> parse_expr(std::vector<Token> &tokens, int &pos, int min_precedence);

static ExprPtr parse_unary(std::vector<Token> &tokens, int &pos);
static ExprPtr parse_primary(std::vector<Token> &tokens, int &pos);

// Estrutura auxiliar para operações binárias
struct OpInfo {
        char op;                // Caractere da operação
        int precedence;         // Precedência da operação (crescente)
        bool right_assoc;       // Associatividade
};

// Precedência maior que +, -, * e / e menor que ^
static const int UNARY_PRECEDENCE = 3;

static bool binop_info(char op, OpInfo &info);

ExprPtr parse_text(std::string src)
{
        std::vector<Token> tokens = lexical_analysis(src);

        if (tokens.size() == 0)
                throw std::runtime_error("Vetor de tokens vazio. Ocorreu um erro durante a análise léxica.");

        ExprPtr expr;
        int pos = 0;

        if (matches_funcdef(tokens))
                expr = parse_funcdef(tokens, pos);
        else
                expr = parse_expr(tokens, pos, 1);

        // Testa se todos os tokens foram consumidos

        Token &tok = peek_token(tokens, pos);

        if (tok.type != TokenType::END)
                throw ParseError("Token inesperado '" + tok.text + "'", tok.pos);

        return expr;
}

static ExprPtr parse_primary(std::vector<Token> &tokens, int &pos)
{
        Token tok = consume_token(tokens, pos);

        switch (tok.type) {
        case TokenType::NUMBER: {
                // Números

                std::unique_ptr<Number> num = std::make_unique<Number>(tok.num);
                return num;
        }
        
        case TokenType::STRING: {
                // variáveis e chamadas de funções
                
                std::string name = tok.text;

                if (peek_token(tokens, pos).type != TokenType::LEFT_PAREN) {
                        std::unique_ptr<Variable> var = std::make_unique<Variable>(std::move(name));
                        return var;
                }

                pos++;

                // Chamada de função (parse de uma tupla)

                Args args;

                tok = peek_token(tokens, pos);

                if (tok.type == TokenType::RIGHT_PAREN) {
                        pos++;
                } else {
                        do {
                                ExprPtr arg = parse_expr(tokens, pos, 1);
                                args.push_back(std::move(arg));

                                tok = consume_token(tokens, pos);
                        } while(tok.type == TokenType::COMMA);

                        if (tok.type != TokenType::RIGHT_PAREN)
                                throw ParseError("Parêntese direito ')' faltante", tok.pos);
                }

                std::unique_ptr<Call> call = std::make_unique<Call>(std::move(name), std::move(args));
                return call;
        }

        case TokenType::LEFT_PAREN: {
                // Parênteses

                ExprPtr expr = parse_expr(tokens, pos, 1);

                consume_token(tokens, pos, TokenType::RIGHT_PAREN, "Parêntese direito ')' faltante");

                return expr;
        }

        default:
                throw ParseError("Expressão faltante", tok.pos);
        }
}

static std::unique_ptr<Expr> parse_expr(std::vector<Token> &tokens, int &pos, int min_precedence)
{
        // Parsing com paradigma bottom-up:
        // a árvore é construída do nó mais à esquerda até o topo

        ExprPtr lhs = parse_unary(tokens, pos);

        while (true) {
                Token &tok = peek_token(tokens, pos);
                OpInfo info;

                bool is_binop = binop_info(tok.text[0], info);

                if (!is_binop || (info.precedence < min_precedence))
                        // A operação possui precedência menor que a mínima
                        // então o parse da expressão termina
                        break;

                pos++;

                ExprPtr rhs;

                // Associatividade à direita e à esquerda

                if (info.right_assoc)
                        rhs = parse_expr(tokens, pos, info.precedence);
                else
                        rhs = parse_expr(tokens, pos, info.precedence + 1);

                // Construção bottom-up

                lhs = std::make_unique<BinOp>(info.op, std::move(lhs), std::move(rhs));
        }

        return lhs;
}

static ExprPtr parse_unary(std::vector<Token> &tokens, int &pos)
{
        Token tok = peek_token(tokens, pos);

        if (tok.type == TokenType::PLUS || tok.type == TokenType::MINUS) {
                // Parse de expressões como +x ou -x

                char op = tok.text[0];
                pos++;
                
                ExprPtr rhs = parse_expr(tokens, pos, UNARY_PRECEDENCE);

                if (op == '-') {
                        std::unique_ptr<UnOp> unop = std::make_unique<UnOp>('-', std::move(rhs));
                        return unop;
                } else {
                        return rhs;
                }
        } else {
                // Parse das demais expressões

                ExprPtr expr = parse_primary(tokens, pos);
                return expr;
        }
}

static bool matches_funcdef(std::vector<Token> &tokens)
{
        // STRING ( STRING , STRING, ... ) = EXPR
        
        // O último token sempre é do tipo TokenType::END, o que 
        // elimina a necessidade de checar para o fim do vetor todas as vezes

        // STRING

        Token tok = tokens[0];

        if (tok.type != TokenType::STRING)
                return false;

        // LEFT_PAREN

        tok = tokens[1];

        if (tok.type != TokenType::LEFT_PAREN)
                return false;

        // STRING, STRING, ... RIGHT_PAREN

        int i = 2;

        do {
                tok = tokens[i];
                i++;

                if (tok.type != TokenType::STRING)
                        return false;

                tok = tokens[i];
                i++;
        } while (tok.type == TokenType::COMMA);

        if (tok.type != TokenType::RIGHT_PAREN)
                return false;

        // EQUALS

        tok = tokens[i];
        i++;

        if (tok.type != TokenType::EQUALS)
                return false;

        // Checar se há "corpo"

        tok = tokens[i];
        i++;

        if (tok.type != TokenType::END)
                return true;
        else
                throw ParseError("Expressão faltante", tok.pos);
}

static std::unique_ptr<FuncDef> parse_funcdef(std::vector<Token> &tokens, int &pos)
{
        // STRING ( STRING , STRING, ... ) = EXPR

        // Nome da função 

        Token tok = consume_token(tokens, pos, TokenType::STRING, "Parâmetro name faltante");

        std::string name = tok.text;

        // LEFT_PAREN

        consume_token(tokens, pos, TokenType::LEFT_PAREN, "Esperado parêntese esquerdo '(' faltante para definição de função");

        // STRING, STRING, ... RIGHT_PAREN

        std::vector<std::string> params;

        do {
                tok = consume_token(tokens, pos, TokenType::STRING, "Parâmetro faltante");

                params.push_back(tok.text);
                
                tok = consume_token(tokens, pos);
        } while (tok.type == TokenType::COMMA);

        if (tok.type != TokenType::RIGHT_PAREN)
                throw ParseError("Parêntese direito ')' faltante", tok.pos);

        // EQUALS

        consume_token(tokens, pos, TokenType::EQUALS, "Sinal de igual '=' faltante");

        // Parse do "corpo" da função

        tok = peek_token(tokens, pos);

        if (tok.type == TokenType::END)
                throw ParseError("Expressão faltante", tok.pos);

        ExprPtr body = parse_expr(tokens, pos, 1);

        // Retorno

        std::unique_ptr<FuncDef> func = std::make_unique<FuncDef>(std::move(name), std::move(params), std::move(body));

        return func;
}

static Token &consume_token(std::vector<Token> &tokens, int &pos, TokenType type, std::string msg)
{
        Token &tok = tokens[pos];

        // Nunca avança além do token END, para que pos continue válido
        if (tok.type != TokenType::END)
                pos++;

        if (type != TokenType::NONE && tok.type != type)
                throw ParseError(msg, tok.pos);

        return tok;
}

static Token &peek_token(std::vector<Token> &tokens, int pos)
{
        Token &tok = tokens[pos];
        return tok;
}

static bool binop_info(char op, OpInfo &info)
{
        switch (op) {
        case '+':
                info = {'+', 1, false};
                return true;
        case '-':
                info = {'-', 1, false};
                return true;
        case '*':
                info = {'*', 2, false};
                return true;
        case '/':
                info = {'/', 2, false};
                return true;
        case '^':
                info = {'^', 4, true};
                return true;
        default:
                return false;
        }
}