#include <iomanip>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

#include "commands.h"
#include "eval.h"

bool command(const std::string &input, FunctionTable &functions, Variables &variables)
{
        if (input == ":sair") {
                return true;
        } else if (input == ":listar") {
                std::vector<decltype(functions.begin())> nativeFunctions;
                std::vector<decltype(functions.begin())> userFunctions;
                
                for (auto it = functions.begin(); it != functions.end(); it++) {
                        if (it->second.native) {
                                nativeFunctions.push_back(it);
                        } else {
                                userFunctions.push_back(it);
                        }
                }

                const auto sortByName = [](const auto &left, const auto &right) {
                        return left->first < right->first;
                };

                std::sort(nativeFunctions.begin(), nativeFunctions.end(), sortByName);
                std::sort(userFunctions.begin(), userFunctions.end(), sortByName);

                std::cout << "Funções nativas:\n";
                
                for (const auto it : nativeFunctions) {
                        std::cout << "  " << it->first << "\n";
                }

                std::cout << "\nFunções cadastradas:\n";
                
                for (const auto it : userFunctions) {
                        std::cout << "  ";
                        it->second.definition->print();
                        std::cout << '\n';
                }

                std::cout << "\nVariáveis do intérprete:\n";

                for (auto it = variables.begin(); it != variables.end(); it++) {
                        const auto &[name, value] = *it;
                        std::cout << "  " << name << " = " << std::setprecision(15) << value << '\n';
                }

                return false;
        } else if (input == ":limpar") {
                // limpa a tela com um ansi escape
                std::cout << "\033[2J\033[H";
                return false;
        } else if (input == ":ajuda") {
                std::cout << "Comandos disponíveis:\n";
                std::cout << "  :sair   - Encerra o programa\n";
                std::cout << "  :listar - Lista funções e variáveis cadastradas\n";
                std::cout << "  :limpar - Limpa a tela\n";
                std::cout << "  :ajuda  - Mostra esta mensagem de ajuda\n";
                return false;
        } else {
                std::cerr << "Comando desconhecido: " << input << '\n';
                return false;
        }
}