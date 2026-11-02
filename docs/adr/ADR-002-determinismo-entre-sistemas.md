# ADR-002 — Determinismo entre sistemas

Estado: **aceito** (Fase 1, 2026-11-02). Substitui a pergunta em aberto "a mesma seed dá a mesma partida em macOS, Linux e Windows?" do ADR-001.

## Contexto

O upstream fixado (`9b7b0062`) compilado sem alteração **não** produz a mesma partida nos três sistemas: as 6 seeds de ouro divergem em 6 de 6 (CI: execução 41443841078). Cada sistema, sozinho, é repetível. Sem igualdade entre sistemas, um save ou replay gerado num sistema não valeria em outro.

## Causas (identificadas por rastreio no CI e confirmadas pelo efeito dos remendos)

1. **Ordem de leitura de diretório.** `file_dir_get_contents` e `file_add_support_directory_recursive` (`src/file.c`) usam `g_dir_read_name`, que devolve na ordem do sistema de arquivos. A ordem de carga dos arquivos de definição muda e, com ela, os dados gerados. Os três sistemas listam em ordens diferentes. Remendo `0001`.
2. **Ordem de avaliação de argumentos indefinida em C.** Dois pontos chamam, na mesma expressão, funções que consomem sorteios: `name_get_from_list` (`src/name.c`, nome e sobrenome em `sprintf`) e `transfer_add_new_players` (`src/transfer.c`, `transfer_add_player(transfer_player_get_new(math_rnd..), math_rndi..)`). O clang (macOS) avalia da esquerda para a direita; o gcc (Linux e MinGW) avaliava em outra ordem. A sequência de sorteios se separa a partir daí. Remendos `0002` e `0003`, que fixam a ordem esquerda→direita.

O rastreio mostrou o caminho (execuções do GitHub Actions em `andre-zaguette/bygfoot-moderno`; lista completa na seção Evidência): atributos numéricos idênticos com nomes diferentes (causa 2, `name.c`); estado inicial idêntico entre Ubuntu e Windows depois do `0001`; contador de sorteios idêntico até o fim de uma partida e 14 sorteios a mais no Ubuntu até a seguinte, no ponto de chamada `transfer.c:317/319` (causa 2, `transfer.c`).

## Decisão

- O produto **compila a partir de uma cópia de `upstream/` com `patches/0001..0003`** (`tools/apply-patches.sh`). `upstream/` continua sem edição.
- A referência de determinismo é `tests/golden-seeds/expected-patched.txt` (6 seeds) mais a bateria `seeds-extended.txt` (16 seeds, 30 semanas). O CI exige hashes iguais nos 3 sistemas (jobs `compare`).
- O resultado do upstream original continua medido, só informativo, e diverge de propósito.
- O sentido escolhido para a ordem de avaliação é o do clang (esquerda→direita): os hashes do macOS não mudaram com os remendos.

## Evidência

A confirmação causal é o efeito dos remendos: `compare` vermelho sem eles, verde com eles. O rastreio localizou onde.

| Execução | O que mostra |
|---|---|
| 41443841078 | 1ª comparação: 6 de 6 seeds divergem entre os 3 sistemas |
| 41445189552 | mesma partida (`fixture_id` 25, mesmos times), lances diferentes desde o 1º; ordem de `readdir` diferente nos 3 sistemas |
| 41448492125 | só com o remendo 0001: Ubuntu e Windows coincidem; macOS diverge em 4 de 6 |
| 41452586037 | resumo do estado inicial: 0 diferenças entre Ubuntu e Windows com 0001; macOS difere |
| 41454672874, 41456927863 | amostra crua: atributos numéricos idênticos, só os **nomes** diferem no macOS; o remendo 0002 iguala os nomes |
| 41459247749 | rastro das chamadas de nome: macOS e Ubuntu iguais com a ordem fixada |
| 41461680615 | com 0001+0002 restam só `value`/`wage` de poucos jogadores (3 times) |
| 41463971487 | decisão de evento por lance: sorteios iguais, limiares `foul`/`poss` diferem em 1 ulp de `float` (macOS×Ubuntu e Ubuntu×Windows) |
| 41466900873, 41469790177 | contador de sorteios: igual até o fim de uma partida, depois 14 sorteios a mais no Ubuntu; ponto de chamada em `transfer.c:317/319` |
| 41472997729 | com 0001+0002+0003, `compare` verde nas 6 seeds de ouro |
| 41476866071 | `compare` verde também na bateria de 16 seeds por 30 semanas (16 hashes distintos, iguais nos 3); o upstream original divergiu em 6 de 6 |
| 41481640595 | teste de contrato `MatchTimeline v1` verde nos 3 sistemas, amostra byte a byte |

As ferramentas de rastreio estão em `tests/diag/` (ver `tests/diag/README.md`); as execuções de diagnóstico terminaram "failure" no job `compare` de propósito (era o sinal).

Hipóteses descartadas: acento no nome do time (a seed com `B. Tórshavn` divergia igual às outras); FMA, testado só localmente no macOS ARM (`CFLAGS=-ffp-contract=off BF_SRC=build/patched-src BF_BUILD=build/patched-nofma sh tools/build-upstream.sh` e `BYGFOOT_BIN=build/patched-nofma/bygfoot python3 -I tools/golden-hash.py`: nenhum hash mudou; sem execução de CI).

**O que o hash das seeds de ouro cobre:** a saída completa de `--format timeline-v1` (lances, jogadores, nomes, comentários, mandante, visitante, placar), com `source.patches` fixado vazio. Não cobre o estado completo do jogo (saves).

## Consequências e riscos

- **Cobertura limitada.** Só os caminhos exercitados foram verificados (22 seeds, 2 a 30 semanas, um país). Podem existir outros pontos de ordem indefinida em código que essas partidas não tocam (por exemplo copas e finais de temporada). Mitigação: toda sincronização com o upstream e todo remendo novo reexecuta as seeds de ouro e a bateria nos 3 sistemas.
- **Ponto flutuante.** `float` e `powf` diferem em 1 ulp entre bibliotecas (visto em `foul` e `poss`, entre macOS e Ubuntu e entre Ubuntu e Windows). Nas seeds testadas isso não mudou nenhuma decisão. Mas `value` e `wage` de 3 jogadores em 3 times de outros países diferiam entre macOS e Ubuntu no estado inicial (`powf` e arredondamento por `rint`). Risco residual: uma decisão com sorteio muito perto de um limiar pode virar em outro sistema. Não corrigido; fica como limitação conhecida.
- Binários compilados sem os remendos não dão a mesma partida entre sistemas. **Medido:** a timeline da partida do usuário (`MatchTimeline v1`) é igual entre sistemas nas seeds testadas, com os remendos. **Não medido:** a igualdade de saves completos. Há diferença conhecida em `value`/`wage` de poucos jogadores no estado inicial entre macOS e Ubuntu (vista com os remendos 0001 e 0002, execução 41461680615). Portanto não prometemos save portável entre sistemas.
- Sete remendos candidatos do ADR-001 continuam sem aplicar; os três acima são os primeiros remendos reais.
