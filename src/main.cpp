#include "commands.h"
#include "eval.h"
#include "lexer.h"
#include "parser.h"
#include "persistencia.h"
#include "plot.h"
#include "raylib.h"

#include <iomanip>
#include <iostream>
#include <string>


int main()
{
        SetTraceLogLevel(LOG_NONE);     // desativa o console da raylib

        FunctionTable &functions = FunctionTable::instance();
        std::string input;

        // carrega as funções salvas antes de criar o saver,
        const std::string save_path = "functions.txt";
        load_functions(save_path);
        FunctionSaver saver(save_path);

        std::cout << "Digite \":ajuda\" para ver os comandos disponíveis.\n";

        while (true) {
                std::cout << "f> ";

                if (!std::getline(std::cin, input))
                        break;

                if (input[0] == ':') {
                        bool exit = command(input);

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
                                functions.add(*definition);

                                // Só chega aqui se add() não lançou exceção
                                saver.save(input);

                                std::cout << "Função cadastrada: ";
                                definition->print();
                                std::cout << '\n';
                        } else {
                                std::cout << std::setprecision(15)
                                          << expr->eval() << '\n';
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


        plot_wait();

        return 0;
}