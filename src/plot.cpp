#include "ast.h"
#include "eval.h"
#include "lexer.h"
#include "parser.h"
#include "plot.h"
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// Comando :plot já analisado
struct PlotCommand {
        std::string func;               // Nome da função
        double inicio;                  // Início do intervalo
        double fim;                     // Fim do intervalo
        int particoes;                  // Número de partições do intervalo
        std::string arquivo;            // Arquivo de saída
};

static const int PARTICOES_PADRAO = 100;
static const int PARTICOES_MAX = 100000;

static std::thread plot_thread;

static PlotCommand plot_parse(const std::string &input,
                              const FunctionTable &functions,
                              const Variables &variables);
static std::vector<double> plot_sample(const PlotCommand &cmd,
                                       const FunctionTable &functions);
static void plot_image(PlotCommand cmd, std::vector<double> ys);

static double eval_limite(const std::string &s, const char *nome,
                          const FunctionTable &functions,
                          const Variables &variables);
static int parse_particoes(const std::string &s);

static bool is_delim(char c);
static std::string trim(const std::string &s);

void plot_async(std::string &input, const FunctionTable &functions,
                const Variables &variables)
{
        // Sintaxe e avaliação na thread principal,
        // assim os erros chegam até a command()

        PlotCommand cmd = plot_parse(input, functions, variables);
        std::vector<double> ys = plot_sample(cmd, functions);

        // Espera o desenho anterior e dispara o novo

        plot_wait();
        plot_thread = std::thread(plot_image, std::move(cmd), std::move(ys));
}

void plot_wait()
{
        if (plot_thread.joinable())
                plot_thread.join();
}

static PlotCommand plot_parse(const std::string &input,
                              const FunctionTable &functions,
                              const Variables &variables)
{
        // :plot f inicio..fim..particoes arquivo.png
        // :plot f inicio..fim "arquivo.png"

        // O arquivo é o último elemento, o intervalo é o penúltimo
        // e o que sobra no início é a função f

        const char *uso = "uso: :plot f inicio..fim[..particoes] arquivo.png";

        if (input.compare(0, 5, ":plot") != 0 || input.size() < 6 || !is_delim(input[5]))
                throw PlotError("Comando inválido");

        std::string resto = trim(input.substr(5));

        if (resto.empty())
                throw PlotError(uso);

        PlotCommand cmd;

        // Arquivo

        if (resto.back() == '"') {
                if (resto.size() < 2)
                        throw PlotError("Aspas não fechadas");

                size_t abre = resto.rfind('"', resto.size() - 2);

                if (abre == std::string::npos)
                        throw PlotError("Aspas não fechadas");

                if (abre > 0 && !is_delim(resto[abre - 1]))
                        throw PlotError("Falta espaço antes das aspas");

                cmd.arquivo = resto.substr(abre + 1, resto.size() - abre - 2);
                resto = trim(resto.substr(0, abre));
        } else {
                size_t sep = resto.find_last_of(" \t");

                if (sep == std::string::npos)
                        throw PlotError(uso);

                cmd.arquivo = resto.substr(sep + 1);
                resto = trim(resto.substr(0, sep));
        }

        if (cmd.arquivo.empty())
                throw PlotError("Nome de arquivo vazio");

        // Intervalo

        size_t sep = resto.find_last_of(" \t");

        if (sep == std::string::npos)
                throw PlotError(uso);

        std::string intervalo = resto.substr(sep + 1);

        // Função

        cmd.func = trim(resto.substr(0, sep));

        if (cmd.func.empty())
                throw PlotError("Função faltante");

        // inicio..fim ou inicio..fim..particoes

        size_t p1 = intervalo.find("..");

        if (p1 == std::string::npos)
                throw PlotError("O intervalo deve ter a forma inicio..fim[..particoes]");

        std::string s_inicio = intervalo.substr(0, p1);
        std::string s_resto = intervalo.substr(p1 + 2);
        std::string s_fim;

        cmd.particoes = PARTICOES_PADRAO;

        size_t p2 = s_resto.find("..");

        if (p2 == std::string::npos) {
                s_fim = s_resto;
        } else {
                s_fim = s_resto.substr(0, p2);
                cmd.particoes = parse_particoes(s_resto.substr(p2 + 2));
        }

        cmd.inicio = eval_limite(s_inicio, "início", functions, variables);
        cmd.fim = eval_limite(s_fim, "fim", functions, variables);

        if (cmd.inicio >= cmd.fim)
                throw PlotError("O início deve ser menor que o fim");

        // Função cadastrada e com um único parâmetro

        if (!functions.contains(cmd.func))
                throw PlotError("Função desconhecida: '" + cmd.func + "'");

        if (functions.get(cmd.func).arity != 1)
                throw PlotError("'" + cmd.func + "' deve receber exatamente 1 argumento");

        return cmd;
}

static std::vector<double> plot_sample(const PlotCommand &cmd,
                                       const FunctionTable &functions)
{
        // Avalia f nos pontos da partição

        std::vector<double> ys;
        ys.reserve(cmd.particoes + 1);

        double passo = (cmd.fim - cmd.inicio) / cmd.particoes;

        for (int i = 0; i <= cmd.particoes; i++) {
                double x = cmd.inicio + i * passo;

                try {
                        ys.push_back(functions.call(cmd.func, {x}));
                } catch (const EvalError &) {
                        // Divisão por zero ou fora do domínio: vira lacuna na curva
                        ys.push_back(NAN);
                }
        }

        return ys;
}

static void plot_image(PlotCommand cmd, std::vector<double> ys)
{
        // Executa na thread paralela: só usa os valores recebidos por cópia

        const int largura = 800;
        const int altura = 600;
        const int margem = 40;

        // Intervalo vertical, ignorando valores não finitos

        double ymin = INFINITY, ymax = -INFINITY;

        for (double y : ys) {
                if (std::isfinite(y)) {
                        ymin = std::min(ymin, y);
                        ymax = std::max(ymax, y);
                }
        }

        if (ymin > ymax) {              // Nenhum valor válido
                ymin = -1;
                ymax = 1;
        }

        if (ymin == ymax) {             // Função constante
                ymin -= 1;
                ymax += 1;
        }

        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        InitWindow(largura, altura, "Plot");

        if (!IsWindowReady()) {
                std::cerr << "Erro: não foi possível criar o contexto gráfico.\n";
                return;
        }

        // Conversão dos índices e valores para coordenadas da imagem

        auto px = [&](int i) {
                return (float)(margem + (largura - 2.0 * margem) * i / cmd.particoes);
        };

        auto py = [&](double y) {
                return (float)(altura - margem - (altura - 2.0 * margem) * (y - ymin) / (ymax - ymin));
        };

        BeginDrawing();
        ClearBackground(WHITE);

        DrawRectangleLines(margem, margem, largura - 2 * margem, altura - 2 * margem, LIGHTGRAY);

        // Eixo y = 0, quando visível

        if (ymin < 0 && ymax > 0)
                DrawLineV(Vector2{(float)margem, py(0)},
                          Vector2{(float)(largura - margem), py(0)}, GRAY);

        // Curva

        for (size_t i = 1; i < ys.size(); i++) {
                if (std::isfinite(ys[i - 1]) && std::isfinite(ys[i]))
                        DrawLineEx(Vector2{px((int)i - 1), py(ys[i - 1])},
                                   Vector2{px((int)i), py(ys[i])}, 2.0f, RED);
        }

        EndDrawing();

        Image img = LoadImageFromScreen();
        ExportImage(img, cmd.arquivo.c_str());

        UnloadImage(img);
        CloseWindow();
}

static double eval_limite(const std::string &s, const char *nome,
                          const FunctionTable &functions,
                          const Variables &variables)
{
        // Avalia um limite do intervalo com o parser e o eval do interpretador
        // Aceita expressões sem espaços: pi, 2*pi, -pi/2, sqrt(2), ...

        if (trim(s).empty())
                throw PlotError(std::string("Valor faltante para ") + nome);

        double v;

        try {
                ExprPtr expr = parse_text(s);

                if (dynamic_cast<const FuncDef *>(expr.get()))
                        throw PlotError(std::string("Definição de função inválida em ") + nome);

                v = expr->eval(functions, variables);
        }
        catch (const LexError &e) {
                throw PlotError(std::string("Erro léxico em ") + nome + ": " + e.what());
        }
        catch (const ParseError &e) {
                throw PlotError(std::string("Erro sintático em ") + nome + ": " + e.what());
        }
        catch (const EvalError &e) {
                throw PlotError(std::string("Erro ao avaliar ") + nome + ": " + e.what());
        }

        if (!std::isfinite(v))
                throw PlotError(std::string("Valor não finito em ") + nome);

        return v;
}

static int parse_particoes(const std::string &s)
{
        if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos)
                throw PlotError("As partições devem ser um inteiro positivo: '" + s + "'");

        // Evita overflow no std::stoi

        if (s.size() > 7)
                throw PlotError("Partições acima do máximo (" + std::to_string(PARTICOES_MAX) + ")");

        int n = std::stoi(s);

        if (n < 1 || n > PARTICOES_MAX)
                throw PlotError("As partições devem estar entre 1 e " + std::to_string(PARTICOES_MAX));

        return n;
}

static bool is_delim(char c)
{
        return c == ' ' || c == '\t';
}

static std::string trim(const std::string &s)
{
        size_t b = s.find_first_not_of(" \t");

        if (b == std::string::npos)
                return "";

        size_t e = s.find_last_not_of(" \t");

        return s.substr(b, e - b + 1);
}