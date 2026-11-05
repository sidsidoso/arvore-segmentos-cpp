// arvore_segmentos.hpp - arvores de segmentos (header-only).
//
// 1) ArvoreSegmentos<T, Op>: generica sobre um MONOIDE (operacao associativa
//    com elemento neutro), iterativa, no layout "bottom-up" de tamanho 2n:
//       folhas em t[n .. 2n-1], no interno i = Op(t[2i], t[2i+1]), raiz t[1].
//    Construcao O(n), consulta de intervalo [l, r) e atualizacao pontual em
//    O(log n), sem recursao. A operacao NAO precisa ser comutativa (a
//    consulta acumula separadamente pela esquerda e pela direita).
//
// 2) ArvoreSegmentosPreguicosa<T>: soma, minimo e maximo de intervalo com
//    ATUALIZACAO DE INTERVALO (somar v a todos os elementos de [l, r)) por
//    propagacao preguicosa (lazy). Implementacao recursiva classica, em
//    heap 1-indexado; a profundidade da recursao e ceil(log2 n) + 1, entao a
//    pilha nunca e problema (~21 chamadas para 1 milhao de elementos).
//
// Todos os intervalos sao semiabertos [l, r), com 0 <= l <= r <= tamanho().
// l == r e o intervalo vazio (resultado = elemento neutro). Intervalos
// invalidos lancam std::out_of_range.
#ifndef ARVORE_SEGMENTOS_HPP
#define ARVORE_SEGMENTOS_HPP

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Monoides prontos. Um monoide e qualquer tipo com:
//   T neutro() const;                     elemento neutro e: op(e, x) == x == op(x, e)
//   T operator()(const T& a, const T& b) const;   associativa
// ---------------------------------------------------------------------------

template <class T>
struct Soma {
    T neutro() const { return T{}; }
    T operator()(const T& a, const T& b) const { return a + b; }
};

template <class T>
struct Minimo {
    T neutro() const { return std::numeric_limits<T>::max(); }
    T operator()(const T& a, const T& b) const { return b < a ? b : a; }
};

template <class T>
struct Maximo {
    T neutro() const { return std::numeric_limits<T>::lowest(); }
    T operator()(const T& a, const T& b) const { return a < b ? b : a; }
};

namespace detalhe_segmentos {

inline void validar_intervalo(std::size_t l, std::size_t r, std::size_t n) {
    if (l > r || r > n) {
        throw std::out_of_range("intervalo [" + std::to_string(l) + ", " + std::to_string(r) +
                                ") invalido para tamanho " + std::to_string(n));
    }
}

inline void validar_indice(std::size_t i, std::size_t n) {
    if (i >= n) {
        throw std::out_of_range("indice " + std::to_string(i) + " fora de [0, " +
                                std::to_string(n) + ")");
    }
}

}  // namespace detalhe_segmentos

// ===========================================================================
// Arvore de segmentos iterativa (bottom-up), generica sobre o monoide Op.
// ===========================================================================
template <class T, class Op = Soma<T>>
class ArvoreSegmentos {
public:
    using value_type = T;
    using size_type = std::size_t;

    // n elementos iguais ao neutro.
    explicit ArvoreSegmentos(size_type n = 0, Op op = Op())
        : n_(n), op_(std::move(op)), t_(2 * n, op_.neutro()) {}

    // Constroi a partir de um vetor em O(n).
    explicit ArvoreSegmentos(const std::vector<T>& valores, Op op = Op())
        : ArvoreSegmentos(valores.begin(), valores.end(), std::move(op)) {}

    // Constroi a partir de um intervalo de iteradores em O(n).
    template <class It, class = typename std::iterator_traits<It>::iterator_category>
    ArvoreSegmentos(It inicio, It fim, Op op = Op()) : op_(std::move(op)) {
        std::vector<T> folhas(inicio, fim);
        n_ = folhas.size();
        t_.resize(2 * n_, op_.neutro());
        std::move(folhas.begin(), folhas.end(), t_.begin() + static_cast<std::ptrdiff_t>(n_));
        // Cada no interno uma unica vez, de baixo para cima: O(n).
        for (size_type i = n_; i-- > 1;) t_[i] = op_(t_[2 * i], t_[2 * i + 1]);
    }

    size_type tamanho() const noexcept { return n_; }
    bool vazia() const noexcept { return n_ == 0; }

    // Valor atual da posicao i. O(1)
    const T& valor(size_type i) const {
        detalhe_segmentos::validar_indice(i, n_);
        return t_[n_ + i];
    }
    const T& operator[](size_type i) const noexcept { return t_[n_ + i]; }

    // Atribui 'v' a posicao i e recalcula so os ancestrais. O(log n)
    void atualizar(size_type i, T v) {
        detalhe_segmentos::validar_indice(i, n_);
        i += n_;
        t_[i] = std::move(v);
        for (i /= 2; i >= 1; i /= 2) t_[i] = op_(t_[2 * i], t_[2 * i + 1]);
    }

    // Op(a[l], a[l+1], ..., a[r-1]) na ordem; neutro se l == r. O(log n)
    //
    // l e r sobem juntos pela arvore. Se l e filho direito, o no l nao divide
    // o pai com o resto do intervalo: entra no acumulador da ESQUERDA e l
    // avanca. Se r (exclusivo) e filho direito, o no r-1 entra no acumulador
    // da DIREITA. Manter os dois lados separados preserva a ordem, por isso
    // monoides nao comutativos (concatenacao, matrizes...) tambem funcionam.
    T consultar(size_type l, size_type r) const {
        detalhe_segmentos::validar_intervalo(l, r, n_);
        T esquerda = op_.neutro();
        T direita = op_.neutro();
        for (l += n_, r += n_; l < r; l /= 2, r /= 2) {
            if (l & 1) esquerda = op_(esquerda, t_[l++]);
            if (r & 1) direita = op_(t_[--r], direita);
        }
        return op_(esquerda, direita);
    }

    // Op de todos os elementos. O(log n)
    T total() const { return consultar(0, n_); }

    // Copia dos valores das folhas. O(n)
    std::vector<T> valores() const {
        return std::vector<T>(t_.begin() + static_cast<std::ptrdiff_t>(n_), t_.end());
    }

private:
    size_type n_ = 0;
    Op op_;
    std::vector<T> t_;  // t_[1] e a raiz (se n e potencia de 2); t_[0] nao e usado
};

// ===========================================================================
// Arvore de segmentos com propagacao preguicosa (lazy):
//   somar(l, r, v)            a[i] += v para todo i em [l, r)      O(log n)
//   soma / minimo / maximo    de [l, r)                            O(log n)
// T deve ser aritmetico (inteiro ou ponto flutuante).
// ===========================================================================
template <class T>
class ArvoreSegmentosPreguicosa {
public:
    using value_type = T;
    using size_type = std::size_t;

    // Resumo de um intervalo. Para o intervalo vazio: soma 0, minimo = maior
    // valor representavel e maximo = menor (os neutros de cada operacao).
    struct Resumo {
        T soma;
        T minimo;
        T maximo;
    };

    explicit ArvoreSegmentosPreguicosa(size_type n = 0)
        : ArvoreSegmentosPreguicosa(std::vector<T>(n, T{})) {}

    explicit ArvoreSegmentosPreguicosa(const std::vector<T>& valores)
        : n_(valores.size()), nos_(n_ == 0 ? 0 : 4 * n_) {
        if (n_ > 0) construir(1, 0, n_, valores);
    }

    size_type tamanho() const noexcept { return n_; }
    bool vazia() const noexcept { return n_ == 0; }

    // Soma v a todos os elementos de [l, r). Intervalo vazio nao faz nada.
    void somar(size_type l, size_type r, T v) {
        detalhe_segmentos::validar_intervalo(l, r, n_);
        if (l < r) somar(1, 0, n_, l, r, v);
    }

    // Atribui v a posicao i (implementado como somar(i, i+1, v - atual)).
    void atribuir(size_type i, T v) {
        T atual = valor(i);
        somar(i, i + 1, v - atual);
    }

    // As consultas nao sao const: ao descer, empurram pendencias para os
    // filhos (o conteudo logico nao muda).
    Resumo resumo(size_type l, size_type r) {
        detalhe_segmentos::validar_intervalo(l, r, n_);
        if (l == r) return vazio();
        return consultar(1, 0, n_, l, r);
    }

    T soma(size_type l, size_type r) { return resumo(l, r).soma; }
    T minimo(size_type l, size_type r) { return resumo(l, r).minimo; }
    T maximo(size_type l, size_type r) { return resumo(l, r).maximo; }

    T valor(size_type i) {
        detalhe_segmentos::validar_indice(i, n_);
        return consultar(1, 0, n_, i, i + 1).soma;
    }

    // Valores atuais de todas as posicoes (empurra todas as pendencias). O(n)
    std::vector<T> valores() {
        std::vector<T> saida;
        saida.reserve(n_);
        if (n_ > 0) coletar(1, 0, n_, saida);
        return saida;
    }

private:
    // Cada no cobre [ini, fim) e guarda o resumo do intervalo JA com as
    // somas pendentes aplicadas; 'pendente' e o que ainda falta repassar
    // aos filhos.
    struct No {
        T soma{};
        T minimo{};
        T maximo{};
        T pendente{};
    };

    static Resumo vazio() {
        return {T{}, std::numeric_limits<T>::max(), std::numeric_limits<T>::lowest()};
    }

    static Resumo juntar(const Resumo& a, const Resumo& b) {
        return {a.soma + b.soma, b.minimo < a.minimo ? b.minimo : a.minimo,
                a.maximo < b.maximo ? b.maximo : a.maximo};
    }

    void recalcular(size_type no) {
        const No& e = nos_[2 * no];
        const No& d = nos_[2 * no + 1];
        nos_[no].soma = e.soma + d.soma;
        nos_[no].minimo = d.minimo < e.minimo ? d.minimo : e.minimo;
        nos_[no].maximo = e.maximo < d.maximo ? d.maximo : e.maximo;
    }

    // Aplica "+v em todo o intervalo do no" em O(1), sem descer.
    void aplicar(size_type no, size_type comprimento, T v) {
        No& x = nos_[no];
        x.soma += v * static_cast<T>(comprimento);
        x.minimo += v;
        x.maximo += v;
        x.pendente += v;
    }

    // Repassa a pendencia do no aos dois filhos (so quando vamos descer).
    void empurrar(size_type no, size_type ini, size_type meio, size_type fim) {
        if (nos_[no].pendente != T{}) {
            aplicar(2 * no, meio - ini, nos_[no].pendente);
            aplicar(2 * no + 1, fim - meio, nos_[no].pendente);
            nos_[no].pendente = T{};
        }
    }

    void construir(size_type no, size_type ini, size_type fim, const std::vector<T>& v) {
        if (fim - ini == 1) {
            nos_[no].soma = nos_[no].minimo = nos_[no].maximo = v[ini];
            return;
        }
        size_type meio = ini + (fim - ini) / 2;
        construir(2 * no, ini, meio, v);
        construir(2 * no + 1, meio, fim, v);
        recalcular(no);
    }

    void somar(size_type no, size_type ini, size_type fim, size_type l, size_type r, T v) {
        if (l <= ini && fim <= r) {  // no totalmente coberto: para aqui
            aplicar(no, fim - ini, v);
            return;
        }
        size_type meio = ini + (fim - ini) / 2;
        empurrar(no, ini, meio, fim);
        if (l < meio) somar(2 * no, ini, meio, l, r, v);
        if (meio < r) somar(2 * no + 1, meio, fim, l, r, v);
        recalcular(no);
    }

    Resumo consultar(size_type no, size_type ini, size_type fim, size_type l, size_type r) {
        if (l <= ini && fim <= r) return {nos_[no].soma, nos_[no].minimo, nos_[no].maximo};
        size_type meio = ini + (fim - ini) / 2;
        empurrar(no, ini, meio, fim);
        if (r <= meio) return consultar(2 * no, ini, meio, l, r);
        if (meio <= l) return consultar(2 * no + 1, meio, fim, l, r);
        return juntar(consultar(2 * no, ini, meio, l, r), consultar(2 * no + 1, meio, fim, l, r));
    }

    void coletar(size_type no, size_type ini, size_type fim, std::vector<T>& saida) {
        if (fim - ini == 1) {
            saida.push_back(nos_[no].soma);
            return;
        }
        size_type meio = ini + (fim - ini) / 2;
        empurrar(no, ini, meio, fim);
        coletar(2 * no, ini, meio, saida);
        coletar(2 * no + 1, meio, fim, saida);
    }

    size_type n_ = 0;
    std::vector<No> nos_;  // heap 1-indexado: filhos de k sao 2k e 2k+1
};

#endif  // ARVORE_SEGMENTOS_HPP
