# ADR-002 — Determinismo entre sistemas

Estado: **aceito** (Fase 1, 2026-11-02). Substitui a pergunta em aberto "a mesma seed dá a mesma partida em macOS, Linux e Windows?" do ADR-001.

## Contexto

O upstream fixado (`9b7b0062`) compilado sem alteração **não** produz a mesma partida nos três sistemas: as 6 seeds de ouro divergem em 6 de 6 (CI: execução 41443841078). Cada sistema, sozinho, é repetível. Sem igualdade entre sistemas, um save ou replay gerado num sistema não valeria em outro.

## Causas (provadas por rastreio no CI, comparando macOS, Ubuntu e Windows)

1. **Ordem de leitura de diretório.** `file_dir_get_contents` e `file_add_support_directory_recursive` (`src/file.c`) usam `g_dir_read_name`, que devolve na ordem do sistema de arquivos. A ordem de carga dos arquivos de definição muda e, com ela, os dados gerados. Os três sistemas listam em ordens diferentes. Remendo `0001`.
2. **Ordem de avaliação de argumentos indefinida em C.** Dois pontos chamam, na mesma expressão, funções que consomem sorteios: `name_get_from_list` (`src/name.c`, nome e sobrenome em `sprintf`) e `transfer_add_new_players` (`src/transfer.c`, `transfer_add_player(transfer_player_get_new(math_rnd..), math_rndi..)`). O clang (macOS) avalia da esquerda para a direita; o gcc (Linux e MinGW) avaliava em outra ordem. A sequência de sorteios se separa a partir daí. Remendos `0002` e `0003`, que fixam a ordem esquerda→direita.

O rastreio mostrou o caminho: atributos numéricos idênticos com nomes diferentes (causa 2, `name.c`); estado inicial idêntico entre Ubuntu e Windows depois do `0001`; contador de sorteios idêntico até o fim de uma partida e 14 sorteios a mais no Ubuntu até a seguinte, no ponto de chamada `transfer.c:317/319` (causa 2, `transfer.c`).

## Decisão

- O produto **compila a partir de uma cópia de `upstream/` com `patches/0001..0003`** (`tools/apply-patches.sh`). `upstream/` continua sem edição.
- A referência de determinismo é `tests/golden-seeds/expected-patched.txt` (6 seeds) mais a bateria `seeds-extended.txt` (16 seeds, 30 semanas). O CI exige hashes iguais nos 3 sistemas (jobs `compare`).
- O resultado do upstream original continua medido, só informativo, e diverge de propósito.
- O sentido escolhido para a ordem de avaliação é o do clang (esquerda→direita): os hashes do macOS não mudaram com os remendos.

## Evidência

- Execução 41472997729: `compare` verde com os 3 remendos nas 6 seeds. Execução 41476866071: `compare` verde também na bateria de 16 seeds por 30 semanas (16 hashes distintos, iguais nos 3 sistemas); o passo do upstream original divergiu em 6 de 6.
- Hipóteses descartadas com teste: FMA (`-ffp-contract=off` não mudou nenhum hash no macOS ARM); acento no nome do time (a seed com `B. Tórshavn` divergia igual às outras).
- Ferramentas de diagnóstico mantidas: `tools/first-divergence.py`, `tools/dump-state.sh`, `tools/state-digest.py`, `tools/state-sample.py` e os remendos só de rastreio em `tests/diag/`.

## Consequências e riscos

- **Cobertura limitada.** Só os caminhos exercitados foram verificados (22 seeds, 2 a 30 semanas, um país). Podem existir outros pontos de ordem indefinida em código que essas partidas não tocam (por exemplo copas e finais de temporada). Mitigação: toda sincronização com o upstream e todo remendo novo reexecuta as seeds de ouro e a bateria nos 3 sistemas.
- **Ponto flutuante.** `float` e `powf` diferem em 1 ulp entre bibliotecas (visto em `foul` e `poss`, entre macOS e Ubuntu e entre Ubuntu e Windows). Nas seeds testadas isso não mudou nenhuma decisão. Mas `value` e `wage` de 3 jogadores em 3 times de outros países diferiam entre macOS e Ubuntu no estado inicial (`powf` e arredondamento por `rint`). Risco residual: uma decisão com sorteio muito perto de um limiar pode virar em outro sistema. Não corrigido; fica como limitação conhecida.
- Binários compilados sem os remendos não são compatíveis entre sistemas. Saves e replays só são portáveis entre builds com os remendos.
- Sete remendos candidatos do ADR-001 continuam sem aplicar; os três acima são os primeiros remendos reais.
