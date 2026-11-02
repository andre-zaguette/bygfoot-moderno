# Resultado do spike headless (Tarefa 5)

**Resultado: o motor original simulou partidas completas sem janela e sem display, com resultado repetível, no macOS.** Isso cobre o critério do G1 "simular sem depender de janela GTK". Ele **não** cobre "compilar sem GTK": o binário continua ligado às bibliotecas GTK (`otool -L`).

## O que mudou em relação ao plano

O plano previa um executável `bf-headless` novo, com `main.c` próprio e `gui_stubs.c`. A leitura do `main.c` mostrou que o upstream **já tem um frontend de console**: `bygfoot --json ARQUIVO` executa comandos de um arquivo JSON e sai, sem `gtk_init`. Por isso o spike é um wrapper em torno do **binário original, sem nenhuma edição em `upstream/`**, e **nenhum stub foi necessário**.

Comandos do console (`json_interface.c:105-116`): `load_bygfoot`, `save_bygfoot`, `dump_bygfoot`, `add_country`, `add_user`, `start_bygfoot`, `simulate_games`, `get_tables`, `get_players`, `get_fixtures`, `cups`, `leagues`. O upstream já traz testes que usam essa interface (`upstream/test/benchmark.sh`, `test-load-save.sh`).

## O que foi construído

- `spike/headless/bf-headless --seed N [--team NOME] [--weeks K] [--country SID]`: monta os comandos, roda o binário, e imprime uma linha JSON por lance da última partida do time do usuário, mais uma linha final com o placar.
- `spike/headless/extract_units.py`: lê o dump e extrai os lances (só no spike, em Python).
- Códigos de saída: 0 ok, 2 argumento inválido, 3 o motor falhou.
- Exemplo de saída (seed 431199, time `V. Gotu`, 2 semanas): 121 lances + 1 linha final; partida `N. Runavík` 3 × 3 `V. Gotu`.

```
{"area": 1, "event": 1, "minute": 0, "possession": 0, "result": [0, 0], "team": 0, "time": 0}
...
{"away": "V. Gotu", "final": [3, 3], "fixture_id": 28, "home": "N. Runavík"}
```

Os lances têm exatamente os campos do `LiveGameUnit` lidos em `live_game_struct.h`. O dump completo também traz, em cada lance, o nome do jogador, o texto do comentário e as estatísticas finais da partida.

## Evidência (comandos rodados nesta sessão)

| Comando | Resultado |
|---|---|
| `sh tests/spike/test-headless.sh` | `OK` (6 grupos, abaixo) |
| `sh tests/spike/test-upstream-locale-bug.sh` | `OK: defeito reproduzido` |
| `sh tools/verify-upstream-pristine.sh 9b7b0062` | `OK` depois de tudo |
| tempo de uma execução (2 semanas) | ver "Medidas" |

Grupos do `test-headless.sh`: mesma seed repete byte a byte; seeds `0` e `2147483647` repetem; `4294967295` é recusada com exit 2 e mensagem; saída idêntica rodando de uma pasta com espaço e acento e de `/`; saída idêntica sob `LC_ALL=pt_BR.UTF-8` e `LC_ALL=C`; duas execuções simultâneas idênticas ao baseline; número de eventos de gol (tipos 10 e 11) igual à soma do placar, e último lance igual ao placar final.

Verifiquei que o teste de locale tem força: tirando o `LC_ALL=C` de uma cópia do wrapper, a execução sob pt_BR falha (exit 3).

## Medidas

Uma execução de `--seed 431199` (carregar país, criar jogo, simular 2 semanas, despejar o estado e extrair os lances) levou o tempo registrado em `/usr/bin/time -p`: 6,45 s (real), 4,76 s (usuário). O dump completo tem cerca de 112 MB em JSON; a maior parte do tempo vai para gerar e ler esse dump, não para simular.

## Candidatos a remendo no upstream (nenhum aplicado)

| # | Onde | Problema | Efeito |
|---|---|---|---|
| 1 | serialização JSON (`json_serialize.c`) | números decimais saem com a vírgula do locale (`44,549…`) | JSON inválido sob `pt_BR`. O wrapper contorna com `LC_ALL=C`. |
| 2 | `main.c:164` (`--random-seed`, `G_OPTION_ARG_INT`) | seed é inteiro com sinal; acima de 2.147.483.647 o motor só avisa e **segue com seed aleatória** (exit 0) | quebra o determinismo em silêncio. O wrapper recusa essas seeds. |
| 3 | `main.c:198-205` (`-s`) | `support_dir[strlen(support_dir)]` é sempre o terminador nulo (deveria ser `strlen - 1`), e o `bygfoot.conf` é procurado antes do `-s` valer | o motor só acha `support_files` em `PACKAGE_DATA_DIR`, `~/.bygfoot` ou na pasta atual. O wrapper cria um link `support_files` na pasta temporária. |
| 4 | `start_end.c:615-619` | partidas sem usuário usam um `LiveGame` descartável | só a **última partida do time do usuário** tem a linha do tempo. Para ver qualquer confronto seria preciso um remendo ou um modo "grava lances". |
| 5 | `dump_bygfoot` | despeja o estado inteiro (112 MB) | falta um comando que devolva só os lances de um jogo. |
| 6 | `lg_commentary` | avisos `didn't find fitting commentary for unit type 15/18` em stderr | ruído, sem efeito na simulação. |
| 7 | `main.c:459-464` (`G_OS_WIN32`) | no Windows o programa redireciona stdout e stderr para `stdout.log` e `stderr.log` na pasta atual | o modo console não escreve nada no stdout. O wrapper lê `stdout.log` como alternativa. |

## O que este spike NÃO provou

- **Mesmo resultado entre sistemas:** cada sistema foi comparado só consigo mesmo. Não comparei a saída do macOS com a do Linux ou do Windows.
- **"Sem GTK" de verdade:** o binário ainda liga com GTK. Extrair uma biblioteca sem GTK é trabalho da Fase 1.
- **Uma partida avulsa por ids** (`--home`/`--away` do plano): o motor simula o calendário de um país, não um confronto isolado.
- **Streaming em tempo real:** os lances saem todos juntos, no fim. A hipótese de que o laço `live_game_create_unit` permite gerar lance a lance (usada no roadmap) segue **não verificada** em execução.
- **Equivalência com a interface gráfica:** não comparei estes resultados com os de uma partida jogada na janela.
- **Dados reais:** usei o país `faroe_islands` (nomes reais de clubes, ver `docs/LEGAL_REVIEW.md`). Isto é um spike, não um conteúdo do produto.

## Verificação em Windows e Linux (CI do GitHub, execução 41377028975)

`.github/workflows/spike.yml` rodou em `macos-latest`, `ubuntu-latest` e `windows-latest` (MSYS2, MinGW64), e os três terminaram em sucesso: build do upstream, `verify-upstream-pristine.sh` e `tests/spike/test-headless.sh`. Foram precisas várias correções no Windows, todas minhas e não do upstream (cada uma foi reproduzida localmente num teste antes de corrigir):

- o verificador do upstream precisava do objeto do commit, que um clone raso não tem (agora usa `tools/upstream-pin.txt`);
- o MSYS2 não trazia `git` nem `cmp` (pacotes `git` e `diffutils`);
- conversão de fim de linha: `.gitattributes` com `* -text`;
- a saída em `stdout.log` (candidato a remendo 7);
- o Python do Windows escrevia o stdout em cp1252 com CRLF (agora UTF-8 com LF).
