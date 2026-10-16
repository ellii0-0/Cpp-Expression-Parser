#include "ast.h"
#include "parser.h"
#include "lexer.h"

Token &consume_token(std::vector<Token> &tokens, int &pos, TokenType type, std::string msg);
Token &peek_token(std::vector<Token> &tokens, int pos);

bool matches_funcdef(std::vector<Token> &tokens);

std::unique_ptr<FuncDef> parse_funcdef(std::vector<Token> &tokens);
std::unique_ptr<Expr> parse_expr(std::vector<Token> &tokens, int &pos, int min_precedence);

ExprPtr parse_unary(std::vector<Token> &tokens, int &pos);
ExprPtr parse_primary(std::vector<Token> &tokens, int &pos);

// Estrutura auxiliar para operações binárias
struct OpInfo {
        char op;                // Caractere da operação
        int precedence;         // Precedência da operação (crescente)
        bool right_assoc;       // Associatividade
};

// Precedência maior que +, -, * e / e menor que ^
const int UNARY_PRECEDENCE = 3;

bool binop_info(char op, OpInfo &info)
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

ExprPtr parse_text(std::string src)
{
        std::vector<Token> tokens = lexical_analysis(src);

        if (tokens.size() == 0)
                throw std::runtime_error("Vetor de tokens vazio. Ocorreu um erro durante a análise léxica.");

        ExprPtr expr;
        int pos = 0;

        if (matches_funcdef(tokens))
                expr = parse_funcdef(tokens);
        else
                expr = parse_expr(tokens, pos, 1);

        return expr;
}

ExprPtr parse_primary(std::vector<Token> &tokens, int &pos)
{
        Token &tok = consume_token(tokens, pos);

        switch (tok.type) {
        case TokenType::NUMBER: {
                // Números

                std::unique_ptr<Number> num = std::make_unique<Number>(tok.num);
                return num;
        }
        
        case TokenType::STRING: {
                // variáveis e chamadas de funções
                
                std::string name = std::move(tok.text);

                tok = consume_token(tokens, pos);

                if (tok.type != TokenType::LEFT_PAREN) {
                        // Variável
                        std::unique_ptr<Variable> var = std::make_unique<Variable>(name);
                        return var;
                }

                // Chamada de função (parse de uma tupla)

                Args args;

                tok = peek_token(tokens, pos);

                if (tok.type != TokenType::RIGHT_PAREN) {
                        do {
                                ExprPtr arg = parse_expr(tokens, pos, 1);
                                args.push_back(std::move(arg));

                                tok = consume_token(tokens, pos);
                        } while(tok.type == TokenType::COMMA);
                }

                if (tok.type != TokenType::RIGHT_PAREN)
                        throw ParseError("Parêntese direito ')' faltante", tok.pos);

                std::unique_ptr<Call> call = std::make_unique<Call>(name, args);
                return call;
        }

        case TokenType::LEFT_PAREN: {
                // Parênteses

                ExprPtr expr = parse_expr(tokens, pos, 1);

                tok = consume_token(tokens, pos);

                if (tok.type != TokenType::RIGHT_PAREN)
                        throw ParseError("Parêntese direito ')' faltante", tok.pos);

                return expr;
        }

        default:
                throw ParseError("Expressão faltante", tok.pos);
        }
}

std::unique_ptr<Expr> parse_expr(std::vector<Token> &tokens, int &pos, int min_precedence)
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

ExprPtr parse_unary(std::vector<Token> &tokens, int &pos)
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

bool matches_funcdef(std::vector<Token> &tokens)
{
        // STRING ( STRING , STRING, ... ) = EXPR
        
        // O último token sempre é do tipo TokenType::END, o que 
        // elimina a necessidade de checar para o fim do vetor todas as vezes

        // STRING

        Token &tok = tokens[0];

        if (tok.type != TokenType::STRING)
                return false;

        // LEFT_PAREN

        tok = tokens[1];

        if (tok.type != TokenType::LEFT_PAREN)
                return false;

        // EXPR, EXPR, ... RIGHT_PAREN

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

std::unique_ptr<FuncDef> parse_funcdef(std::vector<Token> &tokens)
{
        // STRING ( STRING , STRING, ... ) = EXPR
        
        int pos = 0;

        // Nome da função 

        Token &tok = consume_token(tokens, pos, TokenType::STRING, "Parâmetro name faltante");

        std::string name = std::move(tok.text);

        // LEFT_PAREN

        consume_token(tokens, pos, TokenType::LEFT_PAREN, "Esperado parêntese esquerdo '(' para definição de função");

        // EXPR, EXPR, ... RIGHT_PAREN

        Args params;

        do {
                tok = consume_token(tokens, pos, TokenType::STRING, "Parâmetro faltante");

                std::string param = std::move(tok.text);
                
                std::unique_ptr<Variable> var = std::make_unique<Variable>(param);
                params.push_back(std::move(var));
                
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

        std::unique_ptr<FuncDef> func = std::make_unique<FuncDef>(name, params, body);

        return func;
}

Token &consume_token(std::vector<Token> &tokens, int &pos, TokenType type=TokenType::NONE, std::string msg="")
{
        Token &tok = tokens[pos];
        pos++;

        if (type != TokenType::NONE && tok.type == type)
                throw ParseError(msg, tok.pos);

        return tok;
}

Token &peek_token(std::vector<Token> &tokens, int pos)
{
        Token &tok = tokens[pos];
        return tok;
}