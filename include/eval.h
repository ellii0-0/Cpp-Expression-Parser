#ifndef EVAL_H
#define EVAL_H

#include "ast.h"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class EvalError : public std::runtime_error {
public:
        explicit EvalError(const std::string &msg) : std::runtime_error(msg) {}
};

class FunctionTable {
public:
        struct Function {
                std::size_t arity;
                std::function<double(const std::vector<double> &)> native;
                std::unique_ptr<const FuncDef> definition;
        };

        // Única instância da tabela (criada no primeiro uso)
        static FunctionTable &instance();

        // Singleton: proíbe cópia e movimentação
        FunctionTable(const FunctionTable &) = delete;
        FunctionTable &operator=(const FunctionTable &) = delete;
        FunctionTable(FunctionTable &&) = delete;
        FunctionTable &operator=(FunctionTable &&) = delete;

        // Variáveis da tabela (pi e e já vêm cadastradas)
        Variables &variables() { return vars; }
        const Variables &variables() const { return vars; }

        // Busca e lança EvalError se o nome não existir.
        const Function &get(const std::string &name) const;
        bool contains(const std::string &name) const;

        // Valida a definição de função. Lança EvalError se inválida.
        void validate(const FuncDef &definition) const;

        // Valida e guarda uma cópia independente da árvore.
        void add(const FuncDef &definition);

        // Lida com chamadas de nós Call
        double call(const std::string &name,
                    const std::vector<double> &args) const;

        // Limpa os unordered_maps e chama init()
        void reset();

        std::unordered_map<std::string, Function>::const_iterator begin() const;
        std::unordered_map<std::string, Function>::const_iterator end() const;

private:
        FunctionTable() { init(); };

        // Cadastra as funções e constantes nativas
        void init();

        std::unordered_map<std::string, Function> functions;
        Variables vars;

        void validate_body(const Expr &expr,
                           const std::unordered_set<std::string> &params) const;
};

#endif