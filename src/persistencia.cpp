#include "persistencia.h"
#include "parser.h"

#include <iostream>

void load_functions(const std::string &path)
{
        FunctionTable &functions = FunctionTable::instance();

        std::ifstream in(path);

        if (!in)
                return;     

        std::string line;
        std::size_t line_number = 0;

        while (std::getline(in, line)) {
                line_number++;

                if (line.find_first_not_of(" \t\r\n") == std::string::npos)
                        continue;

                try {
                        ExprPtr expr = parse_text(line);
                        const auto *definition = dynamic_cast<const FuncDef *>(expr.get());

                        if (!definition)
                                throw EvalError("a linha não é uma definição de função");

                        functions.add(*definition);
                }
                catch (const std::exception &error) {
                        // LexError, ParseError and EvalError all derive from runtime_error
                        std::cerr << "Aviso: linha " << line_number << " de " << path
                                  << " ignorada: " << error.what() << '\n';
                }
        }
}