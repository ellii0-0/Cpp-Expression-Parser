#include <iomanip>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

#include "commands.h"
#include "eval.h"
#include "persistencia.h"
#include "plot.h"

bool command(std::string &input)
{
        FunctionTable &functions = FunctionTable::instance();
        Variables &variables = functions.variables();
        FunctionSaver &saver = FunctionSaver::instance();

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
                std::cout << "  :ajuda   - Mostra esta mensagem de ajuda\n";
                std::cout << "  :reset   - Remove todas as funções do usuário\n";
                std::cout << "  :limpar  - Limpa a tela\n";
                std::cout << "  :listar  - Lista funções e variáveis cadastradas\n";
                std::cout << "  :sair    - Encerra o programa\n\n";

                std::cout << "Salvar plots:\n";
                std::cout << "  :plot [unária] [início]..[fim](..[partições])? [caminho]\n";

                return false;
        } else if (input == ":reset") {
                plot_wait();
                functions.reset();
                saver.erase();
                std::cout << "Funções e variáveis restauradas.\n";
                return false;
        } else if (input.compare(0, 5, ":plot") == 0 && input.size() > 5 &&
	           isspace(input[5])) {
		try {
			plot_async(input);
		} catch (const PlotError &e) {
			std::cerr << "Erro de sintaxe em :plot: " << e.what() << '\n';
		}
		return false;
	} else {
                std::cerr << "Comando desconhecido: " << input << '\n';
                return false;
        }
}