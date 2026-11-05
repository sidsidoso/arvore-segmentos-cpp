# Árvore de Segmentos em C++

[![CI](https://github.com/sidsidoso/arvore-segmentos-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/sidsidoso/arvore-segmentos-cpp/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
[![Licenca: MIT](https://img.shields.io/badge/licen%C3%A7a-MIT-green.svg)](LICENSE)

Implementação header-only de duas **árvores de segmentos** em C++17:

- **`ArvoreSegmentos<T, Op>`** — genérica sobre qualquer **monoide** (soma, mínimo, máximo, mdc, concatenação, composição de funções...), passado como *functor* no parâmetro de template. Usa o layout **bottom-up de tamanho 2n**: construção **O(n) iterativa**, consulta de intervalo e atualização pontual **iterativas** em O(log n), sem recursão — e funciona mesmo com operações **não comutativas**.
- **`ArvoreSegmentosPreguicosa<T>`** — **propagação preguiçosa (lazy)**: somar um valor a todos os elementos de `[l, r)` e consultar soma, mínimo e máximo de `[l, r)`, tudo em O(log n). Implementação recursiva clássica (profundidade ⌈log2 n⌉ + 1).

A demonstração usa as duas para responder perguntas sobre **vendas diárias** (total da semana, pior dia da quinzena, correção de um lançamento, promoção aplicada a um período inteiro). Projeto da disciplina de Estrutura de Dados.

## Sumário

- [Conceito](#conceito)
- [Layout bottom-up de tamanho 2n](#layout-bottom-up-de-tamanho-2n)
- [Consulta iterativa](#consulta-iterativa)
- [Propagação preguiçosa](#propagação-preguiçosa)
- [Complexidade](#complexidade)
- [Decisões de implementação e otimizações](#decisões-de-implementação-e-otimizações)
- [Estrutura de pastas](#estrutura-de-pastas)
- [Como compilar, executar e testar](#como-compilar-executar-e-testar)
- [Exemplo de saída](#exemplo-de-saída)
- [Referências](#referências)

## Conceito

Problema: dado um vetor `a[0..n)`, responder muitas vezes "quanto vale `a[l] ⊕ a[l+1] ⊕ ... ⊕ a[r-1]`?" enquanto o vetor também muda.

| Abordagem              | Consulta  | Atualização pontual | Atualização de intervalo |
|------------------------|-----------|---------------------|--------------------------|
| Vetor puro             | O(n)      | O(1)                | O(n)                     |
| Somas de prefixo       | O(1)      | O(n)                | O(n)                     |
| **Árvore de segmentos**| O(log n)  | O(log n)            | O(log n) com *lazy*      |

A árvore guarda, em cada nó, o resultado da operação sobre um **segmento** do vetor; a raiz cobre tudo e cada nó divide seu segmento ao meio entre os dois filhos. Qualquer intervalo `[l, r)` é a união de **no máximo 2 nós por nível**, ou seja, O(log n) nós.

A operação `⊕` só precisa ser um **monoide**:

- **associativa**: `(x ⊕ y) ⊕ z = x ⊕ (y ⊕ z)` — permite agrupar os pedaços como a árvore quiser;
- com **elemento neutro** `e`: `e ⊕ x = x ⊕ e = x` — resultado do intervalo vazio e valor inicial dos acumuladores.

| Monoide         | Operação     | Neutro                        |
|-----------------|--------------|-------------------------------|
| `Soma<T>`       | `a + b`      | `0`                           |
| `Minimo<T>`     | `min(a, b)`  | maior valor de `T`            |
| `Maximo<T>`     | `max(a, b)`  | menor valor de `T`            |
| (testes) mdc    | `gcd(a, b)`  | `0`                           |
| (testes) texto  | `a + b` (concatenação, **não comutativa**) | `""` |

## Layout bottom-up de tamanho 2n

Em vez de um heap de 4n posições com recursão, as **folhas ficam em `t[n..2n)`** e cada nó interno `i` (de `n-1` até `1`) é `t[i] = t[2i] ⊕ t[2i+1]`. Para `a = {5, 8, 6, 3, 2, 7, 2, 6}` (soma):

```
                        t[1] = 39
                 /                     \
          t[2] = 22                   t[3] = 17
         /         \                 /         \
   t[4] = 13     t[5] = 9      t[6] = 9      t[7] = 8
    /    \        /    \        /    \        /    \
  t[8]  t[9]  t[10] t[11]  t[12] t[13]  t[14] t[15]
   5     8      6     3      2     7      2     6        <- a[0..8) em t[8..16)

 vetor: [ -, 39, 22, 17, 13, 9, 9, 8, 5, 8, 6, 3, 2, 7, 2, 6 ]   (2n = 16 posições)
```

- **Construção O(n)**: copiar as folhas e fazer `for i = n-1 .. 1: t[i] = t[2i] ⊕ t[2i+1]` — cada nó calculado uma única vez.
- **Atualização pontual**: troca `t[n + i]` e sobe pelos pais (`i /= 2`) recalculando: O(log n).
- Funciona para **qualquer n**, não só potências de 2. Quando n não é potência de 2 alguns nós internos misturam pedaços não contíguos e `t[1]` deixa de ser o total, mas **nenhuma consulta usa esses nós**, porque ela só sobe a partir das folhas dentro do intervalo (e `total()` é uma consulta normal de `[0, n)`).

## Consulta iterativa

Os índices `l` e `r` (semiaberto) começam nas folhas e **sobem juntos**:

```
 T esquerda = neutro, direita = neutro;
 for (l += n, r += n; l < r; l /= 2, r /= 2) {
     if (l é ímpar) esquerda = esquerda ⊕ t[l++];   // l é filho direito: o pai cobre
                                                    // coisa fora do intervalo
     if (r é ímpar) direita  = t[--r] ⊕ direita;    // idem para a borda direita
 }
 return esquerda ⊕ direita;
```

Exemplo `consultar(1, 6)` no vetor acima (`8 + 6 + 3 + 2 + 7 = 26`):

```
 nível das folhas:  l = 9 (ímpar)  -> esquerda = t[9] = 8,  l = 10
                    r = 14 (par)   -> nada
 sobe:              l = 5 (ímpar)  -> esquerda = 8 + t[5] = 17 (6+3), l = 6
                    r = 7 (ímpar)  -> direita  = t[6] = 9 (2+7)
 sobe:              l = 3, r = 3   -> fim
 resultado: 17 + 9 = 26
```

Como os pedaços da **esquerda** entram sempre à direita do acumulador da esquerda e os da **direita** sempre à esquerda do acumulador da direita, a ordem original é preservada: concatenação de strings e composição de funções afins (testadas em todos os intervalos para n de 0 a 40) dão o resultado certo.

## Propagação preguiçosa

Somar `v` a cada elemento de `[l, r)` atualizando folha por folha custaria O(n log n). A ideia *lazy*: quando um nó está **totalmente dentro** do intervalo, atualiza-se só o resumo dele em O(1) e anota-se uma **pendência** "falta somar `v` nos meus filhos"; a pendência só desce quando alguém precisar entrar naquele nó.

Para somar `v` em todo o segmento de um nó com `k` elementos:

```
 soma   += v · k        minimo += v        maximo += v        pendente += v
```

Exemplo com `a = {1, 2, 3, 4, 5, 6, 7, 8}` e `somar(0, 4, 10)`:

```
                 [0,8) soma 36 -> 76
                /                      \
   [0,4) soma 10 -> 50               [4,8) soma 26   (intocado)
   pendente = 10  <- PARA AQUI
     /        \
  [0,2)      [2,4)      <- ainda com os valores antigos; só recebem
  soma 3     soma 7        +10 quando uma consulta/atualização descer
```

Cada operação visita no máximo **4 nós por nível** (os que cortam o intervalo nas bordas e seus irmãos), logo O(log n), independentemente do tamanho do intervalo. A versão aqui mantém **soma, mínimo e máximo ao mesmo tempo**, já que somar uma constante a todos os elementos desloca o mínimo e o máximo pela mesma constante.

A implementação é **recursiva** (descer, empurrar a pendência, recombinar ao voltar), que é a forma mais clara para lazy. A profundidade da recursão é ⌈log2 n⌉ + 1, então não há risco de estouro de pilha (cerca de 21 chamadas aninhadas para 1 milhão de elementos). Usa um heap 1-indexado com 4n posições, limite seguro para qualquer n com divisão pelo meio.

## Complexidade

| Operação                                   | Classe                      | Tempo     | Observação                              |
|--------------------------------------------|-----------------------------|-----------|-----------------------------------------|
| construir a partir de vetor / iteradores   | `ArvoreSegmentos`           | **O(n)**  | iterativa, bottom-up                    |
| `consultar(l, r)`                          | `ArvoreSegmentos`           | O(log n)  | iterativa; vale para monoide não comutativo |
| `atualizar(i, v)` (atribuição pontual)     | `ArvoreSegmentos`           | O(log n)  | iterativa                               |
| `valor(i)` / `operator[]`                  | `ArvoreSegmentos`           | O(1)      | lê a folha                              |
| `total()`                                  | `ArvoreSegmentos`           | O(log n)  |                                         |
| construir                                  | `ArvoreSegmentosPreguicosa` | O(n)      | recursiva                               |
| `somar(l, r, v)` (atualização de intervalo)| `ArvoreSegmentosPreguicosa` | O(log n)  | no máximo 4 nós por nível               |
| `soma` / `minimo` / `maximo` / `resumo`    | `ArvoreSegmentosPreguicosa` | O(log n)  | empurra pendências ao descer            |
| `valor(i)` / `atribuir(i, v)`              | `ArvoreSegmentosPreguicosa` | O(log n)  |                                         |
| `valores()`                                | ambas                       | O(n)      |                                         |
| **Espaço**                                 | iterativa / preguiçosa      | **2n** / 4n nós | 4 campos por nó na preguiçosa     |

## Decisões de implementação e otimizações

- **Monoide como functor no template** (`ArvoreSegmentos<T, Op = Soma<T>>`): a operação é resolvida em tempo de compilação e pode ser *inlined*, sem ponteiro de função nem `std::function`. Qualquer tipo com `neutro()` e `operator()` serve; o functor pode ter estado (é guardado por valor).
- **Layout 2n em vez de 4n**: metade da memória do heap recursivo, sem posições desperdiçadas, e índices calculados só com `*2`, `/2` e `+1`. A construção visita cada nó uma vez (O(n)) — inserir as n folhas uma a uma custaria O(n log n) (os testes conferem que os dois caminhos dão o mesmo resultado).
- **Consulta com dois acumuladores**: além de evitar recursão, mantém a ordem dos pedaços e por isso aceita operações não comutativas — a armadilha clássica de quem escreve `res = op(res, t[l])` e `res = op(res, t[r])` no mesmo acumulador.
- **Intervalos semiabertos `[l, r)`**: `l == r` é o intervalo vazio (resultado = neutro, nenhum caso especial), e intervalos adjacentes se juntam sem ±1. Intervalos inválidos (`l > r` ou `r > n`) e índices fora do vetor lançam `std::out_of_range`.
- **Preguiçosa com resumo triplo** (soma, mínimo, máximo): uma única árvore responde as três perguntas; aplicar `+v` a um nó é O(1) para os três campos.
- **Pendência só é empurrada quando necessário** (ao descer num nó parcialmente coberto), e só se for diferente de zero. Por isso as consultas da preguiçosa não são `const`: o conteúdo lógico não muda, mas pendências descem.
- **`atribuir(i, v)` na preguiçosa** reaproveita a soma em intervalo: `somar(i, i+1, v - valor(i))`.
- **Testes contra força bruta**: mais de 100 mil operações aleatórias para cada monoide (soma, mínimo, máximo) e para a preguiçosa, sorteando de propósito intervalos **vazios, unitários, encostados em 0 e em n** e o vetor inteiro, em vários tamanhos (1, 2, 3, potências de 2 e vizinhos como 1023/1024/1025); todos os intervalos possíveis para n de 0 a 40 com concatenação; monoides personalizados (mdc, composição de funções afins mod p, `double`); e 20 mil atualizações quase do tamanho do vetor em 1 milhão de elementos.

## Estrutura de pastas

```
arvore-segmentos-cpp/
├── include/
│   └── arvore_segmentos.hpp        # as duas arvores + monoides (header-only)
├── src/
│   └── main.cpp                    # demonstração com vendas diárias
├── tests/
│   └── test_arvore_segmentos.cpp   # testes (assert + força bruta como oráculo)
├── .github/workflows/ci.yml        # CI: testes com sanitizers + demo
├── Makefile
├── LICENSE
└── README.md
```

## Como compilar, executar e testar

### Linux / macOS (make)

```bash
make            # compila demo e testes em build/
make run        # executa a demonstração
make test       # executa os testes
make test SANITIZE=1   # testes com AddressSanitizer + UndefinedBehaviorSanitizer
make clean
```

### Windows (g++ do MinGW-w64 / WinLibs)

```bat
mkdir build
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude src/main.cpp -o build/demo.exe
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude tests/test_arvore_segmentos.cpp -o build/testes.exe
build\demo.exe
build\testes.exe
```

Com `mingw32-make` também funcionam `mingw32-make run` e `mingw32-make test` (o alvo `clean` usa `rm`, disponível no Git Bash).

### Uso

```cpp
#include "arvore_segmentos.hpp"

std::vector<int> vendas = {120, 95, 143, 80, 210};
ArvoreSegmentos<int> total(vendas);                    // soma (padrão)
ArvoreSegmentos<int, Minimo<int>> pior(vendas);
int semana = total.consultar(0, 5);                    // [0, 5)
total.atualizar(3, 100);                               // corrige o dia 4

ArvoreSegmentosPreguicosa<int> lote(vendas);
lote.somar(1, 4, 25);                                  // +25 nos dias 2 a 4
int maior = lote.maximo(0, 5);

// Monoide próprio: basta neutro() e operator()
struct Produto { long long neutro() const { return 1; }
                 long long operator()(long long a, long long b) const { return a * b; } };
ArvoreSegmentos<long long, Produto> prod(std::vector<long long>{2, 3, 4});
```

## Exemplo de saída

`make run` (os tempos da seção "Escala" **variam por máquina**):

```
Layout bottom-up (n = 8, vetor de 2n posicoes; no i = t[2i] + t[2i+1]):
                         [1] 39
              [2] 22                  [3] 17
        [4] 13       [5] 9       [6] 9       [7] 8
folhas: [8]5 [9]8 [10]6 [11]3 [12]2 [13]7 [14]2 [15]6
consultar(1, 6) = 8+6+3+2+7 = 26

Vendas diarias (30 dias):
  dias  1- 7:  145  154  138  136  121  197  237
  dias  8-14:  140  100  119  107  153  204  184
  dias 15-21:  142  118  157  123  148  209  207
  dias 22-28:  156  139  111  140  133  228  198
  dias 29-30:  109  120

Consultas (dias numerados de 1 a 30, intervalos fechados):
  semana 1 (dias  1- 7): total 1128, pior dia 121, melhor dia 237
  semana 2 (dias  8-14): total 1007, pior dia 100, melhor dia 204
  semana 3 (dias 15-21): total 1104, pior dia 118, melhor dia 209
  semana 4 (dias 22-28): total 1105, pior dia 111, melhor dia 228
  mes inteiro: total 4573, media 152.4 por dia
  dias 10-20: total 1664, minimo 107

Correcao: dia 15 tinha 142, o valor certo e 20 (atualizacao pontual)
  dias 10-20: total 1542, minimo 20

Promocao: +25 unidades em todos os dias 8-21 (somar em [7, 21))
  dias 10-20: total 1817, minimo 45, maximo 234
Devolucao: -10 unidades em todo o mes (somar em [0, 30))
  mes inteiro: total 4501, minimo 35, maximo 227
Vendas finais:
  dias  1- 7:  135  144  128  126  111  187  227
  dias  8-14:  155  115  134  122  168  219  199
  dias 15-21:   35  133  172  138  163  224  222
  dias 22-28:  146  129  101  130  123  218  188
  dias 29-30:   99  110

Escala (n = 1.000.000; tempos variam por maquina):
  iterativa, construcao O(n):                               6.9 ms
  iterativa, 1e6 operacoes (atualizar / consultar):       110.0 ms
  preguicosa, 1e6 operacoes (somar em [l,r) / soma):      671.3 ms
  (soma de verificacao: 144655855173990)
```

A diferença entre as duas últimas linhas ilustra o custo da recursão e das pendências: a preguiçosa resolve um problema mais difícil (atualização de **intervalo**), mas a iterativa de 2n é bem mais rápida quando só há atualizações pontuais.

`make test`:

```
[ok] exemplo pequeno (soma, minimo, maximo)
[ok] arvore vazia, unitaria e intervalos invalidos
[ok] todos os intervalos com monoide nao comutativo (n = 0..40)
[ok] construcao O(n) equivale a n atualizacoes
[ok] soma: 100 mil operacoes vs forca bruta
[ok] minimo e maximo: 100 mil operacoes vs forca bruta
[ok] monoides personalizados (mdc, funcoes afins, double)
[ok] preguicosa: exemplo, vazia e excecoes
[ok] preguicosa: 100 mil operacoes vs forca bruta
[ok] preguicosa: 1 milhao de elementos
10 testes passaram
```

## Referências

- CORMEN, T. H.; LEISERSON, C. E.; RIVEST, R. L.; STEIN, C. *Algoritmos: Teoria e Prática*. 3. ed. Elsevier, 2012. Capítulo 14 (Aumentando estruturas de dados).
- DE BERG, M.; CHEONG, O.; VAN KREVELD, M.; OVERMARS, M. *Computational Geometry: Algorithms and Applications*. 3. ed. Springer, 2008. Seção 10.3 (Segment Trees).
- HALIM, S.; HALIM, F. *Competitive Programming 3*. 2013. Seção 2.4.3 (Segment Tree).
- LAAKSONEN, A. *Competitive Programmer's Handbook*. 2018. Capítulo 9 (Range queries) e Seção 28.1 (Segment trees revisited — lazy propagation). Disponível em: <https://cses.fi/book/book.pdf>.
- AL.CASH. *Efficient and easy segment trees*. Codeforces, 2015. Disponível em: <https://codeforces.com/blog/entry/18051>.
- cp-algorithms: [Segment Tree](https://cp-algorithms.com/data_structures/segment_tree.html).

## Licença

Distribuído sob a licença MIT. Veja [LICENSE](LICENSE).
