#include "commands.h"
#include "eval.h"
#include "lexer.h"
#include "parser.h"

#include <iomanip>
#include <iostream>
#include <string>

int main()
{
        FunctionTable functions;
        std::string input;

        Variables variables;

        variables["pi"] = 3.14159265358979323846;
        variables["e"] = 2.71828182845904523536;

        std::cout << "Digite \":ajuda\" para ver os comandos disponíveis.\n";

        while (true) {
                std::cout << "f> ";
                std::getline(std::cin, input);

                if (input[0] == ':') {
                        bool exit = command(input, functions, variables);
        
                        if (exit)
                                break;
                        else
                                continue;
                }
                        
                if (input.find_first_not_of(" \t\r\n") == std::string::npos)
                        continue;

                try {
                        ExprPtr expr = parse_text(input);

                        if (const auto *definition = dynamic_cast<const FuncDef *>(expr.get())) {
                                functions.add(*definition, variables);
                                std::cout << "Função cadastrada: ";
                                definition->print();
                                std::cout << '\n';
                        } else {
                                std::cout << std::setprecision(15)
                                          << expr->eval(functions, variables) << '\n';
                        }
                }
                catch (const LexError &error) {
                        std::cerr << "Erro léxico na posição " << error.pos()
                                  << ": " << error.what() << '\n';
                }
                catch (const ParseError &error) {
                        std::cerr << "Erro sintático na posição " << error.pos()
                                  << ": " << error.what() << '\n';
                }
                catch (const EvalError &error) {
                        std::cerr << "Erro de avaliação: " << error.what() << '\n';
                }
        }
}