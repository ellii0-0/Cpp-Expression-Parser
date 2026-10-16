#include "eval.h"

#include <cmath>
#include <unordered_map>
#include <utility>

namespace {

double checked(double value)
{
        if (std::isnan(value))
                throw EvalError("Resultado fora do domínio dos números reais");
        if (!std::isfinite(value))
                throw EvalError("Resultado não finito ou fora do intervalo de double");
        return value;
}

void check_arity(const std::string &name, std::size_t expected,
                 std::size_t received)
{
        if (expected != received)
                throw EvalError("Função '" + name + "' espera " +
                                std::to_string(expected) + " argumento(s), recebeu " +
                                std::to_string(received));
}

// unique_ptr não pode ser copiado: cada nó precisa de uma nova instância.
ExprPtr clone(const Expr &expr)
{
        if (const auto *node = dynamic_cast<const Number *>(&expr))
                return std::make_unique<Number>(node->get_val());

        if (const auto *node = dynamic_cast<const Variable *>(&expr))
                return std::make_unique<Variable>(node->get_name());

        if (const auto *node = dynamic_cast<const UnOp *>(&expr))
                return std::make_unique<UnOp>(node->get_op(), clone(node->get_rhs()));

        if (const auto *node = dynamic_cast<const BinOp *>(&expr))
                return std::make_unique<BinOp>(node->get_op(),
                                              clone(node->get_lhs()),
                                              clone(node->get_rhs()));

        if (const auto *node = dynamic_cast<const Call *>(&expr)) {
                Args args;
                for (const auto &arg : node->get_args()) {
                        if (!arg)
                                throw EvalError("Argumento nulo na árvore");
                        args.push_back(clone(*arg));
                }
                return std::make_unique<Call>(node->get_name(), std::move(args));
        }

        throw EvalError("Tipo de expressão inválido no corpo da função");
}

} // namespace

FunctionTable::FunctionTable()
{
        auto unary = [this](const std::string &name, double (*operation)(double)) {
                functions.emplace(name, Function{
                        1,
                        [operation](const std::vector<double> &args) {
                                return operation(args[0]);
                        },
                        nullptr
                });
        };

        unary("sen", static_cast<double (*)(double)>(std::sin));
        unary("sin", static_cast<double (*)(double)>(std::sin));
        unary("cos", static_cast<double (*)(double)>(std::cos));
        unary("tan", static_cast<double (*)(double)>(std::tan));
        unary("tg", static_cast<double (*)(double)>(std::tan));
        unary("sqrt", static_cast<double (*)(double)>(std::sqrt));
        unary("abs", static_cast<double (*)(double)>(std::fabs));
        unary("exp", static_cast<double (*)(double)>(std::exp));
        unary("ln", static_cast<double (*)(double)>(std::log));
        unary("log", static_cast<double (*)(double)>(std::log));
        unary("log10", static_cast<double (*)(double)>(std::log10));
        unary("asin", static_cast<double (*)(double)>(std::asin));
        unary("acos", static_cast<double (*)(double)>(std::acos));
        unary("atan", static_cast<double (*)(double)>(std::atan));

        functions.emplace("pow", Function{
                2, [](const std::vector<double> &args) {
                        return std::pow(args[0], args[1]);
                }, nullptr
        });

        functions.emplace("atan2", Function{
                2, [](const std::vector<double> &args) {
                        return std::atan2(args[0], args[1]);
                }, nullptr
        });
}

bool FunctionTable::contains(const std::string &name) const
{
        return functions.find(name) != functions.end();
}

const FunctionTable::Function &FunctionTable::get(const std::string &name) const
{
        auto it = functions.find(name);
        if (it == functions.end())
                throw EvalError("Função não encontrada: '" + name + "'");
        return it->second;
}

void FunctionTable::validate(const FuncDef &definition, const Variables &variables) const
{
        if (contains(definition.get_name()))
                throw EvalError("Função já cadastrada: '" + definition.get_name() + "'");

        std::unordered_set<std::string> params;
        for (const auto &param : definition.get_params()) {
                if (!params.insert(param).second)
                        throw EvalError("Parâmetro repetido: '" + param + "'");
        }

        // A função nova ainda não está na tabela: autorreferência é rejeitada.
        validate_body(definition.get_body(), params, variables);
}

void FunctionTable::validate_body(
        const Expr &expr, const std::unordered_set<std::string> &params,
        const std::unordered_map<std::string, double> &variables) const
{
        if (const auto *node = dynamic_cast<const Number *>(&expr)) {
                checked(node->get_val());
                return;
        }

        if (const auto *node = dynamic_cast<const Variable *>(&expr)) {
                if (variables.find(node->get_name()) == variables.end() &&
                    params.find(node->get_name()) == params.end())
                        throw EvalError("Variável livre não permitida: '" +
                                        node->get_name() + "'");
                return;
        }

        if (const auto *node = dynamic_cast<const UnOp *>(&expr)) {
                if (node->get_op() != '+' && node->get_op() != '-')
                        throw EvalError("Operador unário inválido");
                validate_body(node->get_rhs(), params, variables);
                return;
        }

        if (const auto *node = dynamic_cast<const BinOp *>(&expr)) {
                if (std::string("+-*/^").find(node->get_op()) == std::string::npos)
                        throw EvalError("Operador binário inválido");
                validate_body(node->get_lhs(), params, variables);
                validate_body(node->get_rhs(), params, variables);
                return;
        }

        if (const auto *node = dynamic_cast<const Call *>(&expr)) {
                const Function &function = get(node->get_name());
                check_arity(node->get_name(), function.arity, node->get_args().size());
                for (const auto &arg : node->get_args()) {
                        if (!arg)
                                throw EvalError("Argumento nulo na árvore");
                        validate_body(*arg, params, variables);
                }
                return;
        }

        throw EvalError("Tipo de expressão inválido no corpo da função");
}

void FunctionTable::add(const FuncDef &definition, Variables &variables)
{
        validate(definition, variables);

        auto copy = std::make_unique<FuncDef>(
                definition.get_name(), definition.get_params(),
                clone(definition.get_body()));

        // só insere depois que toda a validação e a cópia deram certo.
        functions.emplace(definition.get_name(), Function{
                definition.get_params().size(), {}, std::move(copy)
        });
}

double FunctionTable::call(const std::string &name,
                           const std::vector<double> &args) const
{
        const Function &function = get(name);
        check_arity(name, function.arity, args.size());
        for (double value : args)
                checked(value);

        if (function.native)
                return checked(function.native(args));

        Variables locals;
        const auto &params = function.definition->get_params();
        for (std::size_t i = 0; i < params.size(); i++)
                locals.emplace(params[i], args[i]);

        return function.definition->get_body().eval(*this, locals);
}

double Number::eval(const FunctionTable &, const Variables &) const
{
        return checked(get_val());
}

double Variable::eval(const FunctionTable &, const Variables &variables) const
{
        auto it = variables.find(get_name());
        
        if (it == variables.end())
                throw EvalError("Variável não encontrada: '" + get_name() + "'");
        
        return checked(it->second);
}

double UnOp::eval(const FunctionTable &functions, const Variables &variables) const
{
        double value = get_rhs().eval(functions, variables);

        switch (get_op()) {
        case '+':
                return value;
        case '-':
                return -value;
        default:
                throw EvalError("Operador unário inválido");
        }
}

double BinOp::eval(const FunctionTable &functions, const Variables &variables) const
{
        double left = get_lhs().eval(functions, variables);
        double right = get_rhs().eval(functions, variables);
        
        switch (get_op()) {
        case '+':
                return checked(left + right);
        case '-':
                return checked(left - right);
        case '*':
                return checked(left * right);
        case '/':
                if (right == 0.0)
                        throw EvalError("Divisão por zero");
                
                return checked(left / right);
        case '^':
                return checked(std::pow(left, right));
        default:
                throw EvalError("Operador binário inválido");
        }
}

double Call::eval(const FunctionTable &functions, const Variables &variables) const
{
        const auto &function = functions.get(get_name());
        
        check_arity(get_name(), function.arity, get_args().size());
        
        std::vector<double> values;
        
        for (const auto &arg : get_args()) {
                if (!arg)
                        throw EvalError("Argumento nulo na árvore");
                
                values.push_back(arg->eval(functions, variables));
        }
        
        if (function.native)
                return functions.call(get_name(), values);

        Variables locals = variables;
        const auto &params = function.definition->get_params();
        
        for (std::size_t i = 0; i < params.size(); i++)
                locals.insert_or_assign(params[i], checked(values[i]));

        return function.definition->get_body().eval(functions, locals);
}

double FuncDef::eval(const FunctionTable &, const Variables &) const
{
        throw EvalError("Use FunctionTable::add() para cadastrar uma definição");
}