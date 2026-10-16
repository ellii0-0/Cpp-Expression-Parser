#ifndef EVAL_H
#define EVAL_H

#include "ast.h"

#include <functional>
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

        // funções matemáticas fixas.
        FunctionTable();

        // Busca e lança EvalError se o nome não existir.
        const Function &get(const std::string &name) const;
        bool contains(const std::string &name) const;

        // Valida a definição de função. Lança EvalError se inválida.
        void validate(const FuncDef &definition, const Variables &variables) const;

        // Valida e guarda uma cópia independente da árvore.
        void add(const FuncDef &definition, Variables &variables);

        double call(const std::string &name,
                    const std::vector<double> &args) const;

        std::unordered_map<std::string, Function>::const_iterator begin() const;
        std::unordered_map<std::string, Function>::const_iterator end() const;

private:
        std::unordered_map<std::string, Function> functions;

        void validate_body(const Expr &expr,
                           const std::unordered_set<std::string> &params,
                           const Variables &variables) const;
};

#endif
