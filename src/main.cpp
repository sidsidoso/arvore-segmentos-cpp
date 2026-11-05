// Demonstracao: vendas diarias de um mes consultadas com arvores de segmentos.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <utility>
#include <vector>

#include "arvore_segmentos.hpp"

namespace {

using Relogio = std::chrono::steady_clock;

double ms_desde(Relogio::time_point inicio) {
    return std::chrono::duration<double, std::milli>(Relogio::now() - inicio).count();
}

void imprimir_vendas(const std::vector<int>& vendas) {
    for (std::size_t semana = 0; semana * 7 < vendas.size(); ++semana) {
        std::printf("  dias %2zu-%2zu:", semana * 7 + 1, std::min(vendas.size(), semana * 7 + 7));
        for (std::size_t d = semana * 7; d < vendas.size() && d < semana * 7 + 7; ++d) {
            std::printf(" %4d", vendas[d]);
        }
        std::printf("\n");
    }
}

// Mostra como as folhas e os nos internos ficam no vetor de tamanho 2n.
void desenhar_layout() {
    std::vector<int> v = {5, 8, 6, 3, 2, 7, 2, 6};
    ArvoreSegmentos<int> st(v);
    std::printf("Layout bottom-up (n = 8, vetor de 2n posicoes; no i = t[2i] + t[2i+1]):\n");
    std::printf("                         [1] %d\n", st.total());
    std::printf("              [2] %d                  [3] %d\n", st.consultar(0, 4),
                st.consultar(4, 8));
    std::printf("        [4] %d       [5] %d       [6] %d       [7] %d\n", st.consultar(0, 2),
                st.consultar(2, 4), st.consultar(4, 6), st.consultar(6, 8));
    std::printf("folhas:");
    for (std::size_t i = 0; i < v.size(); ++i) std::printf(" [%zu]%d", i + 8, st[i]);
    std::printf("\nconsultar(1, 6) = 8+6+3+2+7 = %d\n\n", st.consultar(1, 6));
}

}  // namespace

int main() {
    desenhar_layout();

    // Vendas (em unidades) de 30 dias, geradas com semente fixa.
    std::mt19937 rng(2026);
    std::vector<int> vendas(30);
    for (std::size_t d = 0; d < vendas.size(); ++d) {
        int base = (d % 7 == 5 || d % 7 == 6) ? 180 : 100;  // fim de semana vende mais
        vendas[d] = base + static_cast<int>(rng() % 60);
    }
    std::printf("Vendas diarias (30 dias):\n");
    imprimir_vendas(vendas);

    ArvoreSegmentos<int> soma(vendas);                // construcao O(n)
    ArvoreSegmentos<int, Minimo<int>> minimo(vendas);
    ArvoreSegmentos<int, Maximo<int>> maximo(vendas);

    std::printf("\nConsultas (dias numerados de 1 a 30, intervalos fechados):\n");
    for (std::size_t semana = 0; semana < 4; ++semana) {
        std::size_t l = semana * 7, r = l + 7;
        std::printf("  semana %zu (dias %2zu-%2zu): total %4d, pior dia %3d, melhor dia %3d\n",
                    semana + 1, l + 1, r, soma.consultar(l, r), minimo.consultar(l, r),
                    maximo.consultar(l, r));
    }
    std::printf("  mes inteiro: total %d, media %.1f por dia\n", soma.total(),
                soma.total() / static_cast<double>(vendas.size()));
    std::printf("  dias 10-20: total %d, minimo %d\n", soma.consultar(9, 20),
                minimo.consultar(9, 20));

    std::printf("\nCorrecao: dia 15 tinha %d, o valor certo e 20 (atualizacao pontual)\n",
                soma.valor(14));
    soma.atualizar(14, 20);
    minimo.atualizar(14, 20);
    maximo.atualizar(14, 20);
    std::printf("  dias 10-20: total %d, minimo %d\n", soma.consultar(9, 20),
                minimo.consultar(9, 20));

    // Atualizacao em lote com a arvore preguicosa.
    vendas[14] = 20;
    ArvoreSegmentosPreguicosa<int> lote(vendas);
    std::printf("\nPromocao: +25 unidades em todos os dias 8-21 (somar em [7, 21))\n");
    lote.somar(7, 21, 25);
    std::printf("  dias 10-20: total %d, minimo %d, maximo %d\n", lote.soma(9, 20),
                lote.minimo(9, 20), lote.maximo(9, 20));
    std::printf("Devolucao: -10 unidades em todo o mes (somar em [0, 30))\n");
    lote.somar(0, 30, -10);
    std::printf("  mes inteiro: total %d, minimo %d, maximo %d\n", lote.soma(0, 30),
                lote.minimo(0, 30), lote.maximo(0, 30));
    std::printf("Vendas finais:\n");
    imprimir_vendas(lote.valores());

    // Escala: 1 milhao de dias, 1 milhao de operacoes mistas.
    const std::size_t n = 1000000;
    std::vector<long long> grande(n);
    for (auto& x : grande) x = static_cast<long long>(rng() % 1000);
    auto t0 = Relogio::now();
    ArvoreSegmentos<long long> st(grande);
    double t_construcao = ms_desde(t0);
    t0 = Relogio::now();
    long long verificacao = 0;
    for (std::size_t k = 0; k < n; ++k) {
        std::size_t a = rng() % n, b = rng() % n;
        if (a > b) std::swap(a, b);
        if (k % 2 == 0) {
            st.atualizar(a, static_cast<long long>(rng() % 1000));
        } else {
            verificacao += st.consultar(a, b + 1);
        }
    }
    double t_iterativa = ms_desde(t0);

    ArvoreSegmentosPreguicosa<long long> pg(grande);
    t0 = Relogio::now();
    for (std::size_t k = 0; k < n; ++k) {
        std::size_t a = rng() % n, b = rng() % n;
        if (a > b) std::swap(a, b);
        if (k % 2 == 0) {
            pg.somar(a, b + 1, static_cast<long long>(rng() % 21) - 10);
        } else {
            verificacao += pg.soma(a, b + 1);
        }
    }
    double t_preguicosa = ms_desde(t0);

    std::printf("\nEscala (n = 1.000.000; tempos variam por maquina):\n");
    std::printf("  iterativa, construcao O(n):                          %8.1f ms\n", t_construcao);
    std::printf("  iterativa, 1e6 operacoes (atualizar / consultar):    %8.1f ms\n", t_iterativa);
    std::printf("  preguicosa, 1e6 operacoes (somar em [l,r) / soma):   %8.1f ms\n", t_preguicosa);
    std::printf("  (soma de verificacao: %lld)\n", verificacao);
    return 0;
}
