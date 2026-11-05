// Testes das arvores de segmentos. Usa assert e forca bruta como oraculo.
#undef NDEBUG
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "arvore_segmentos.hpp"

namespace {

int testes_passados = 0;

template <class F>
void executar(const char* nome, F teste) {
    teste();
    ++testes_passados;
    std::cout << "[ok] " << nome << '\n';
}

using i64 = long long;

// ---------- oraculos de forca bruta ----------

template <class T, class Op>
T bruto(const std::vector<T>& v, std::size_t l, std::size_t r, Op op) {
    T acc = op.neutro();
    for (std::size_t i = l; i < r; ++i) acc = op(acc, v[i]);
    return acc;
}

// Monoide NAO comutativo: concatenacao. Se a consulta trocasse a ordem dos
// pedacos, o resultado sairia embaralhado.
struct Concatenacao {
    std::string neutro() const { return {}; }
    std::string operator()(const std::string& a, const std::string& b) const { return a + b; }
};

// Monoide personalizado: maximo divisor comum (neutro 0).
struct Mdc {
    i64 neutro() const { return 0; }
    i64 operator()(i64 a, i64 b) const { return std::gcd(a, b); }
};

// Monoide nao comutativo "de verdade": composicao de funcoes afins
// f(x) = a*x + b (mod P). (f o g) aplicada na ordem do vetor.
struct Afim {
    i64 a, b;
    bool operator==(const Afim& o) const { return a == o.a && b == o.b; }
};
struct ComporAfim {
    static constexpr i64 P = 1000000007;
    Afim neutro() const { return {1, 0}; }
    // primeiro aplica f, depois g: g(f(x)) = g.a*(f.a*x + f.b) + g.b
    Afim operator()(const Afim& f, const Afim& g) const {
        return {g.a * f.a % P, (g.a * f.b + g.b) % P};
    }
};

template <class Op, class Gerar>
void comparar_aleatorio(std::size_t n, int operacoes, std::uint32_t semente, Op op, Gerar gerar) {
    std::mt19937 rng(semente);
    using T = decltype(gerar(rng));
    std::vector<T> v(n);
    for (auto& x : v) x = gerar(rng);
    ArvoreSegmentos<T, Op> st(v, op);
    assert(st.tamanho() == n);
    for (int k = 0; k < operacoes; ++k) {
        int tipo = static_cast<int>(rng() % 10);
        if (tipo < 4 && n > 0) {  // atualizacao pontual
            std::size_t i = rng() % n;
            v[i] = gerar(rng);
            st.atualizar(i, v[i]);
            assert(st.valor(i) == v[i]);
        } else {
            std::size_t l, r;
            switch (tipo) {
                case 4:  // vazio
                    l = r = rng() % (n + 1);
                    break;
                case 5:  // unitario
                    if (n == 0) { l = r = 0; break; }
                    l = rng() % n;
                    r = l + 1;
                    break;
                case 6:  // encostado nos limites
                    l = (rng() % 2) ? 0 : rng() % (n + 1);
                    r = (rng() % 2) ? n : l + rng() % (n - l + 1);
                    break;
                default:  // qualquer
                    l = rng() % (n + 1);
                    r = rng() % (n + 1);
                    if (l > r) std::swap(l, r);
            }
            assert(st.consultar(l, r) == bruto(v, l, r, op));
        }
    }
    assert(st.total() == bruto(v, 0, n, op));
    assert(st.valores() == v);
}

void teste_exemplo_pequeno() {
    std::vector<int> v = {5, 8, 6, 3, 2, 7, 2, 6};
    ArvoreSegmentos<int> soma(v);
    ArvoreSegmentos<int, Minimo<int>> mn(v);
    ArvoreSegmentos<int, Maximo<int>> mx(v);
    assert(soma.total() == 39 && mn.total() == 2 && mx.total() == 8);
    assert(soma.consultar(1, 4) == 17);  // 8 + 6 + 3
    assert(mn.consultar(0, 4) == 3 && mx.consultar(3, 6) == 7);
    soma.atualizar(3, 10);
    mn.atualizar(4, 9);
    assert(soma.consultar(1, 4) == 24 && soma.total() == 46);
    assert(mn.consultar(3, 6) == 3 && mn.consultar(4, 6) == 7);
    assert(soma.valor(3) == 10 && soma[3] == 10);
}

void teste_vazia_e_limites() {
    ArvoreSegmentos<int> vazia;
    assert(vazia.vazia() && vazia.tamanho() == 0);
    assert(vazia.consultar(0, 0) == 0 && vazia.total() == 0);
    ArvoreSegmentos<int, Minimo<int>> mn(std::vector<int>{});
    assert(mn.total() == std::numeric_limits<int>::max());  // neutro do minimo

    ArvoreSegmentos<int> um(std::vector<int>{42});
    assert(um.total() == 42 && um.consultar(0, 1) == 42 && um.consultar(1, 1) == 0);
    um.atualizar(0, -1);
    assert(um.total() == -1);

    ArvoreSegmentos<int> n_zeros(5);  // 5 elementos neutros
    assert(n_zeros.tamanho() == 5 && n_zeros.total() == 0);

    ArvoreSegmentos<int> st(std::vector<int>{1, 2, 3});
    bool lancou = false;
    try { st.consultar(2, 1); } catch (const std::out_of_range&) { lancou = true; }
    assert(lancou);
    lancou = false;
    try { st.consultar(0, 4); } catch (const std::out_of_range&) { lancou = true; }
    assert(lancou);
    lancou = false;
    try { st.atualizar(3, 1); } catch (const std::out_of_range&) { lancou = true; }
    assert(lancou);
    lancou = false;
    try { (void)st.valor(3); } catch (const std::out_of_range&) { lancou = true; }
    assert(lancou);
    assert(st.consultar(3, 3) == 0);  // vazio no fim e valido
}

void teste_todos_intervalos_nao_comutativo() {
    // Para cada n de 0 a 40 (potencias de 2 e nao potencias), todos os
    // intervalos [l, r): a concatenacao precisa sair na ordem certa.
    for (std::size_t n = 0; n <= 40; ++n) {
        std::vector<std::string> v(n);
        for (std::size_t i = 0; i < n; ++i) v[i] = std::string(1, static_cast<char>('a' + i % 26));
        ArvoreSegmentos<std::string, Concatenacao> st(v);
        for (std::size_t l = 0; l <= n; ++l) {
            for (std::size_t r = l; r <= n; ++r) {
                assert(st.consultar(l, r) == bruto(v, l, r, Concatenacao{}));
            }
        }
    }
}

void teste_construcao_equivale_atualizacoes() {
    std::mt19937 rng(3);
    std::vector<i64> v(1000);
    for (auto& x : v) x = static_cast<i64>(rng() % 1000) - 500;
    ArvoreSegmentos<i64> construida(v);              // O(n)
    ArvoreSegmentos<i64> incremental(v.size());       // n atualizacoes O(log n)
    for (std::size_t i = 0; i < v.size(); ++i) incremental.atualizar(i, v[i]);
    for (std::size_t l = 0; l <= v.size(); l += 37) {
        for (std::size_t r = l; r <= v.size(); r += 53) {
            assert(construida.consultar(l, r) == incremental.consultar(l, r));
        }
    }
    ArvoreSegmentos<i64> por_iteradores(v.begin() + 10, v.begin() + 20);
    assert(por_iteradores.tamanho() == 10);
    assert(por_iteradores.total() == std::accumulate(v.begin() + 10, v.begin() + 20, i64{0}));
}

void teste_aleatorio_soma() {
    auto gerar = [](std::mt19937& r) { return static_cast<i64>(r() % 2000001) - 1000000; };
    comparar_aleatorio(1000, 100000, 1, Soma<i64>{}, gerar);
    for (std::size_t n : {1u, 2u, 3u, 7u, 64u, 127u, 1025u}) {
        comparar_aleatorio(n, 5000, static_cast<std::uint32_t>(n), Soma<i64>{}, gerar);
    }
}

void teste_aleatorio_minimo_maximo() {
    auto gerar = [](std::mt19937& r) { return static_cast<int>(r() % 2001) - 1000; };
    comparar_aleatorio(999, 100000, 2, Minimo<int>{}, gerar);
    comparar_aleatorio(1024, 100000, 3, Maximo<int>{}, gerar);
    for (std::size_t n : {1u, 2u, 5u, 33u}) {
        comparar_aleatorio(n, 3000, static_cast<std::uint32_t>(n + 10), Minimo<int>{}, gerar);
        comparar_aleatorio(n, 3000, static_cast<std::uint32_t>(n + 20), Maximo<int>{}, gerar);
    }
}

void teste_monoides_personalizados() {
    auto gerar_mdc = [](std::mt19937& r) { return static_cast<i64>((r() % 50 + 1) * 6); };
    comparar_aleatorio(500, 20000, 4, Mdc{}, gerar_mdc);
    auto gerar_afim = [](std::mt19937& r) {
        return Afim{static_cast<i64>(r() % 1000), static_cast<i64>(r() % 1000)};
    };
    comparar_aleatorio(300, 20000, 5, ComporAfim{}, gerar_afim);
    comparar_aleatorio(257, 20000, 6, ComporAfim{}, gerar_afim);
    auto gerar_real = [](std::mt19937& r) { return static_cast<double>(r() % 1000) / 8.0; };
    comparar_aleatorio(100, 5000, 7, Maximo<double>{}, gerar_real);
}

// ---------- arvore preguicosa ----------

void conferir_preguicosa(ArvoreSegmentosPreguicosa<i64>& st, const std::vector<i64>& v,
                         std::size_t l, std::size_t r) {
    auto res = st.resumo(l, r);
    if (l == r) {
        assert(res.soma == 0);
        assert(res.minimo == std::numeric_limits<i64>::max());
        assert(res.maximo == std::numeric_limits<i64>::lowest());
        return;
    }
    assert(res.soma == std::accumulate(v.begin() + static_cast<std::ptrdiff_t>(l),
                                       v.begin() + static_cast<std::ptrdiff_t>(r), i64{0}));
    assert(res.minimo == *std::min_element(v.begin() + static_cast<std::ptrdiff_t>(l),
                                           v.begin() + static_cast<std::ptrdiff_t>(r)));
    assert(res.maximo == *std::max_element(v.begin() + static_cast<std::ptrdiff_t>(l),
                                           v.begin() + static_cast<std::ptrdiff_t>(r)));
    assert(st.soma(l, r) == res.soma && st.minimo(l, r) == res.minimo &&
           st.maximo(l, r) == res.maximo);
}

void sortear_intervalo(std::mt19937& rng, std::size_t n, std::size_t& l, std::size_t& r) {
    switch (rng() % 6) {
        case 0:  // vazio
            l = r = rng() % (n + 1);
            break;
        case 1:  // unitario
            l = rng() % n;
            r = l + 1;
            break;
        case 2:  // tudo
            l = 0;
            r = n;
            break;
        default:
            l = rng() % (n + 1);
            r = rng() % (n + 1);
            if (l > r) std::swap(l, r);
    }
}

void comparar_preguicosa(std::size_t n, int operacoes, std::uint32_t semente) {
    std::mt19937 rng(semente);
    std::vector<i64> v(n);
    for (auto& x : v) x = static_cast<i64>(rng() % 2001) - 1000;
    ArvoreSegmentosPreguicosa<i64> st(v);
    for (int k = 0; k < operacoes; ++k) {
        std::size_t l, r;
        sortear_intervalo(rng, n, l, r);
        int tipo = static_cast<int>(rng() % 10);
        if (tipo < 4) {
            i64 delta = static_cast<i64>(rng() % 201) - 100;
            st.somar(l, r, delta);
            for (std::size_t i = l; i < r; ++i) v[i] += delta;
        } else if (tipo == 4) {
            std::size_t i = rng() % n;
            i64 novo = static_cast<i64>(rng() % 2001) - 1000;
            st.atribuir(i, novo);
            v[i] = novo;
        } else if (tipo == 5) {
            std::size_t i = rng() % n;
            assert(st.valor(i) == v[i]);
        } else {
            conferir_preguicosa(st, v, l, r);
        }
    }
    assert(st.valores() == v);
}

void teste_preguicosa_exemplo() {
    ArvoreSegmentosPreguicosa<int> st(std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8});
    assert(st.soma(0, 8) == 36 && st.minimo(0, 8) == 1 && st.maximo(0, 8) == 8);
    st.somar(2, 6, 10);  // 1 2 13 14 15 16 7 8
    assert(st.soma(0, 8) == 76 && st.soma(2, 4) == 27 && st.soma(5, 7) == 23);
    assert(st.minimo(2, 6) == 13 && st.maximo(0, 8) == 16 && st.minimo(0, 8) == 1);
    st.somar(0, 8, -1);  // 0 1 12 13 14 15 6 7
    assert(st.valor(0) == 0 && st.valor(5) == 15 && st.soma(6, 8) == 13);
    st.somar(3, 3, 1000);  // vazio: nada muda
    assert(st.soma(0, 8) == 68);
    st.atribuir(7, 100);
    assert(st.maximo(0, 8) == 100 && st.soma(7, 8) == 100);
    assert((st.valores() == std::vector<int>{0, 1, 12, 13, 14, 15, 6, 100}));

    ArvoreSegmentosPreguicosa<int> vazia;
    assert(vazia.vazia() && vazia.soma(0, 0) == 0 && vazia.valores().empty());
    vazia.somar(0, 0, 5);
    bool lancou = false;
    try { vazia.somar(0, 1, 5); } catch (const std::out_of_range&) { lancou = true; }
    assert(lancou);
    lancou = false;
    try { (void)st.soma(5, 9); } catch (const std::out_of_range&) { lancou = true; }
    assert(lancou);
    lancou = false;
    try { (void)st.valor(8); } catch (const std::out_of_range&) { lancou = true; }
    assert(lancou);

    ArvoreSegmentosPreguicosa<double> reais(4);  // 4 zeros
    reais.somar(1, 3, 0.5);
    assert(reais.soma(0, 4) == 1.0 && reais.maximo(0, 4) == 0.5 && reais.minimo(0, 4) == 0.0);
}

void teste_preguicosa_aleatoria() {
    comparar_preguicosa(1000, 100000, 11);
    for (std::size_t n : {1u, 2u, 3u, 4u, 5u, 17u, 1023u, 1024u, 1025u}) {
        comparar_preguicosa(n, 5000, static_cast<std::uint32_t>(n * 7 + 1));
    }
}

void teste_preguicosa_grande() {
    // 1 milhao de elementos, atualizacoes cobrindo quase tudo: cada operacao
    // continua O(log n) gracas a preguica.
    const std::size_t n = 1000000;
    ArvoreSegmentosPreguicosa<i64> st(n);
    std::mt19937 rng(99);
    i64 total = 0;
    for (int k = 0; k < 20000; ++k) {
        std::size_t l = rng() % 100, r = n - rng() % 100;
        i64 d = static_cast<i64>(rng() % 11) - 5;
        st.somar(l, r, d);
        total += d * static_cast<i64>(r - l);
    }
    assert(st.soma(0, n) == total);
}

}  // namespace

int main() {
    executar("exemplo pequeno (soma, minimo, maximo)", teste_exemplo_pequeno);
    executar("arvore vazia, unitaria e intervalos invalidos", teste_vazia_e_limites);
    executar("todos os intervalos com monoide nao comutativo (n = 0..40)",
             teste_todos_intervalos_nao_comutativo);
    executar("construcao O(n) equivale a n atualizacoes", teste_construcao_equivale_atualizacoes);
    executar("soma: 100 mil operacoes vs forca bruta", teste_aleatorio_soma);
    executar("minimo e maximo: 100 mil operacoes vs forca bruta", teste_aleatorio_minimo_maximo);
    executar("monoides personalizados (mdc, funcoes afins, double)", teste_monoides_personalizados);
    executar("preguicosa: exemplo, vazia e excecoes", teste_preguicosa_exemplo);
    executar("preguicosa: 100 mil operacoes vs forca bruta", teste_preguicosa_aleatoria);
    executar("preguicosa: 1 milhao de elementos", teste_preguicosa_grande);
    std::cout << testes_passados << " testes passaram\n";
    return 0;
}
