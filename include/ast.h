#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class FunctionTable;                                            // Tabela de funções
using Variables = std::unordered_map<std::string, double>;      // Tabela de variáveis

// Classe base da árvore de sintaxe (nunca instanciada por si própria)
class Expr {
public:
        virtual ~Expr() = default;              // Destrutor padrão virtual
        virtual void print() const = 0;         // Função print virtual (com override nas demais classes)
        // Função eval virtual (com override nas demais classes)
        virtual double eval(const FunctionTable &, const Variables &) const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;          // Ponteiro único a expressões
using Args = std::vector<ExprPtr>;              // Tupla de argumentos

// Classe de um nó de variáveis simbólicas
class Variable : public Expr {
private:
        // Nome da variável
        std::string name;
public:
        // Construtor padrão do nó de variável
        // Utiliza-se move para não duplicar o buffer da string
        explicit Variable(std::string name) : name(std::move(name)) {}
        const std::string &get_name() const { return name; }                    // Getter do nome
        void print() const override;                                            // Sobrecarga do print para variáveis
        double eval(const FunctionTable &, const Variables &) const override;   // Sobrecarga do eval para variáveis
};

// Classe de um nó de números de pontos flutuantes
class Number : public Expr {
private:
        double val;                                     // Número de 64 bits
public:
        explicit Number(double val) : val(val) {}                               // Construtor padrão do nó de número
        double get_val() const { return val; }                                  // Getter do valor do número
        void print() const override;                                            // Sobrecarga do print para números
        double eval(const FunctionTable &, const Variables &) const override;   // Sobrecarga do eval para números
};

// Classe de um nó de operação binária (+, -, *, /, ^)
class BinOp : public Expr {
private:
        char op;                        // Caractere da operação
        ExprPtr lhs, rhs;               // Lado direito e lado esquerdo
public:
        // Construtor simples com operações de std::move de std::unique_ptr
        BinOp(char op, ExprPtr lhs, ExprPtr rhs)
                : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

        char get_op() const { return op; }                                      // Getter do caractere da operação
        const Expr &get_lhs() const { return *lhs; }                            // Getter da referência do lado esquerdo
        const Expr &get_rhs() const { return *rhs; }                            // Getter da referência do lado direito
        void print() const override;                                            // Sobrecarga do print para operações binárias
        double eval(const FunctionTable &, const Variables &) const override;   // Sobrecarga do eval para operações binárias
};

// Classe de um nó de + e - unários
class UnOp : public Expr {
private:
        char op;                        // Caractere da operação
        ExprPtr rhs;                    // Lado direito
public:
        // Construtor simples com operações de std::move de std::unique_ptr
        UnOp(char op, ExprPtr rhs)
                : op(op), rhs(std::move(rhs)) {}

        char get_op() const { return op; }                                      // Getter do caractere da operação
        const Expr &get_rhs() const { return *rhs; }                            // Getter da referência do lado direito
        void print() const override;                                            // Sobrecarga do print para operações binárias
        double eval(const FunctionTable &, const Variables &) const override;   // Sobrecarga do eval para operações binárias
};

// Classe de chamada de função
// Exemplos: sqrt(x), cos(2*x), atan2(y,x)
class Call : public Expr {
private:
        std::string name;               // Nome da função
        Args args;                      // Argumentos da chamada
public:
        Call(std::string name, Args args)
                : name(std::move(name)), args(std::move(args)) {}               // Construtor simples com std::move
        const std::string &get_name() const { return name; }                    // Getter do nome
        const Args &get_args() const { return args; }                           // Getter da referência dos argumentos
        void print() const override;                                            // Sobrecarga do print para chamadas
        double eval(const FunctionTable &, const Variables &) const override;   // Sobrecarga do eval para chamadas
};

// Definição de funções 
// Exemplos: f(x) = sqrt(x+1), g(x) = 2*x+1, h(x,y) = abs(x-y)
class FuncDef : public Expr {
private:
        std::string name;                       // Nome da função
        std::vector<std::string> params;        // Parâmetros da função
        ExprPtr body;                           // Corpo da função (lei de formação)
public:
        FuncDef(std::string name, std::vector<std::string> params, ExprPtr body)
                : name(std::move(name)), params(std::move(params)),
                  body(std::move(body)) {}                                      // Construtor simples com std::move
        const std::string &get_name() const { return name; }                    // Getter do nome
        const std::vector<std::string> &get_params() const { return params; }   // Getter da referência dos parâmetros
        const Expr &get_body() const { return *body; }                          // Getter da referência do corpo ou lei
        void print() const override;                                            // Sobrecarga do print para definições
        double eval(const FunctionTable &, const Variables &) const override;   // Sobrecarga do eval para definições (apenas uma exceção)
};

// Estrutura auxiliar para operações binárias
struct OpInfo {
        char op;                // Caractere da operação
        int precedence;         // Precedência da operação (crescente)
        bool right_assoc;       // Associatividade
};

// Precedência maior que +, -, * e / e menor que ^
const int UNARY_PRECEDENCE = 3;

// Recebe uma operação binária e retorna as suas informações respectivas
bool binop_info(char op, OpInfo &info);

#endif