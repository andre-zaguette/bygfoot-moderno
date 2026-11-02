# Engenharia reversa do Bygfoot (upstream fixado em `9b7b0062`)

Estado: **em andamento (Fase 0)**. Seções 1–3 escritas na Tarefa 4. Seções 4–5 (SQLite e campeonato) vêm da Tarefa 6. Seção 6 (spike e decisão) vem da Tarefa 8.

Tudo abaixo vem de leitura do código em `upstream/` e de `tools/gtk-coupling.sh`. Nada foi compilado nem executado ainda. Onde há inferência, está marcado como **hipótese**.

## 1. Mapa de `upstream/src` por responsabilidade

201 arquivos `.c` e `.h` (comando: `sh tools/gtk-coupling.sh upstream/src`). Agrupamento por nome de arquivo; a confirmação de cada grupo vem da leitura das tabelas abaixo.

| Responsabilidade | Arquivos |
|---|---|
| Simulação da partida | `live_game.c` (1.996 linhas), `lg_commentary.c`, `game.c` (1.277), `maths.c`, `strategy.c` |
| Calendário e campeonato | `fixture.c` (1.964), `league.c` (1.773), `cup.c`, `competition.c`, `table.c`, `start_end.c` (1.151) |
| Times, jogadores e carreira | `team.c`, `player.c`, `user.c`, `training.c`, `youth_academy.c`, `transfer.c`, `finance.c`, `job.c`, `news.c`, `bet.c`, `stat.c`, `name.c` |
| Persistência | `sqlite_data_store.c`, `load_save.c`, `json_serialize.c`, `json_deserialize.c`, `json_interface.c`, `xml_loadsave_*.c` (formato antigo) |
| Dados de definição | `xml_league.c`, `xml_cup.c`, `xml_team.c`, `xml_country.c`, `xml_strategy.c`, `xml_name.c`, `xml_lg_commentary.c` e outros `xml_*.c` |
| Interface GTK | `gui.c`, `game_gui.c`, `window.c`, `interface.c`, `treeview*.c`, `*_callbacks.c`, `*_callback_func.c`, `*_interface.c`, `option_gui.c`, `support.c` |
| Entrada | `main.c`, `bygfoot.c` |

Dados fora de `src/`: `support_files/definitions` (ligas, copas, times), `support_files/names`, `lg_commentary`, `strategy`, `news` e arquivos `.glade` da interface.

## 2. Acoplamento com a interface, por arquivo

Colunas: `gtk` = linhas que usam a API (`gtk_`, `GTK_`, `Gtk*`, `gdk_`, `Gdk*`); `gui-includes` = `#include` de gui/callbacks/window/treeview; `params GUI*gui` = linhas com `GUI *gui`; `extern` = linhas que começam com `extern` (todas dão 0, então as variáveis globais não são declaradas assim; ver seção 3).

Totais: **56 de 201 arquivos usam a API do GTK**, 40 incluem cabeçalhos de interface, 11 recebem `GUI *gui` por parâmetro. A contagem ingênua de "gtk" dava 193, mas era inflada pelo comentário `GTK2-based` do cabeçalho de quase todos os arquivos. O script foi corrigido com teste (`tools/test-gtk-coupling.sh`).

Arquivos do núcleo (sem os de interface):

| arquivo | gtk | gui-includes | params GUI*gui | extern |
|---|---:|---:|---:|---:|
| `bet.c` | 2 | 2 | 0 | 0 |
| `bygfoot.c` | 0 | 1 | 0 | 0 |
| `competition.c` | 0 | 0 | 0 | 0 |
| `cup.c` | 0 | 0 | 0 | 0 |
| `finance.c` | 0 | 2 | 0 | 0 |
| `fixture.c` | 0 | 0 | 0 | 0 |
| `game.c` | 0 | 2 | 0 | 0 |
| `job.c` | 0 | 0 | 0 | 0 |
| `json_deserialize.c` | 0 | 0 | 0 | 0 |
| `json_serialize.c` | 0 | 0 | 0 | 0 |
| `league.c` | 0 | 0 | 0 | 0 |
| `lg_commentary.c` | 0 | 0 | 0 | 0 |
| `live_game.c` | 3 | 5 | 13 | 0 |
| `load_save.c` | 2 | 4 | 0 | 0 |
| `main.c` | 7 | 3 | 0 | 0 |
| `maths.c` | 0 | 0 | 0 | 0 |
| `misc.c` | 0 | 0 | 0 | 0 |
| `name.c` | 0 | 0 | 0 | 0 |
| `news.c` | 0 | 0 | 0 | 0 |
| `option.c` | 0 | 0 | 0 | 0 |
| `player.c` | 0 | 2 | 0 | 0 |
| `sqlite_data_store.c` | 0 | 0 | 0 | 0 |
| `start_end.c` | 0 | 2 | 0 | 0 |
| `stat.c` | 0 | 0 | 0 | 0 |
| `strategy.c` | 0 | 0 | 0 | 0 |
| `team.c` | 0 | 2 | 0 | 0 |
| `training.c` | 0 | 0 | 0 | 0 |
| `transfer.c` | 0 | 3 | 0 | 0 |
| `user.c` | 1 | 3 | 0 | 0 |
| `youth_academy.c` | 0 | 0 | 0 | 0 |

Leitura: a maior parte do núcleo não chama GTK. Os pontos de contato reais são `live_game.c` (13 parâmetros `GUI *gui`, 3 usos de GTK), `load_save.c`, `main.c`, `bet.c`, `user.c` e vários `#include` de cabeçalhos de interface em arquivos que talvez não usem nada deles (**hipótese**: muitos desses includes são resíduo e podem ser removidos ou ignorados por stubs de cabeçalho).

## 3. Cadeia de chamadas da partida e onde ela toca a interface

### Quem chama a simulação

`live_game_calculate_fixture(Fixture *, LiveGame *, Bygfoot *)` (`live_game.c:67`) é chamada de:

- `start_end.c:615` e `:619`, `:652`: processa todas as partidas da rodada. Para times sem usuário usa um `LiveGame` novo e descartável; para o time do usuário usa o `LiveGame` dele.
- `callback_func.c:88` e `:116`: partida do usuário disparada pela interface.
- `debug.c:287`: simulação em lote com habilidades fixas (modo de depuração).

### O que a função faz (`live_game.c:67-135`)

1. Valida e rearruma a escalação (`team_is_lineup_valid`, `team_rearrange`).
2. `live_game_initialize` (se não estiver retomando uma partida pausada).
3. Laço: `live_game_create_unit` + `live_game_evaluate_unit` até o evento `LIVE_GAME_EVENT_END_MATCH` ou até a interface sinalizar pausa/mudança.
4. No fim: `game_post_match(fix)`, reseta o status da interface.

Cada volta do laço produz uma `LiveGameUnit` (`live_game_struct.h`): posse, área (defesa, meio, ataque), minuto, tempo, placar e evento. Não há coordenadas.

### Pontos em que a simulação toca a interface

| Local | O que toca | Comportamento com `gui == NULL` |
|---|---|---|
| `live_game.c:82,88,102,112,115` | `gui_get_status`, `gui_set_status` | `gui.c:183-196`: retorna `STATUS_NONE` e não faz nada se `gui` é nulo |
| `live_game.c:131` | `gui_should_show_live_game` | `gui.c:211`: retorna `FALSE` se `gui` é nulo |
| `live_game.c:140-145` | `window.live`, `window_create`, `GTK_WINDOW` | só executa quando `show` é verdadeiro |
| `live_game.c:355,554,1018` | `misc_callback_pause_live_game(gui)` | **não verificado** (ainda não li o corpo) |
| `live_game.c:782,1255` | `gui_get_status(gui)` | idem às linhas acima |
| `live_game.c:1645` | `treeview_show_game_stats(... window.live ...)` | **não verificado** |

**Achado principal:** o próprio upstream já trata `gui == NULL` como "sem interface" nos acessores de status (`gui_get_status`, `gui_set_status`, `gui_should_show_live_game`). Isso indica que o caminho das partidas sem usuário foi pensado para não precisar de janela. É um bom sinal para o gate G1, mas **não prova** que `bygfoot->gui = NULL` basta: faltam verificar `misc_callback_pause_live_game`, a linha 1645 e a inicialização global.

### Estado global

O código usa variáveis globais por macros. Verificado:

- `stat2` é macro de `status[2]` (`bygfoot.h:92`). `stat5` é usada em `live_game.c` do mesmo modo (**hipótese**: segue o mesmo padrão; não li a definição). `stat0` em `live_game_calculate_fixture` é só variável local.
- `usr(i)` é `g_array_index(users, User, i)` (`user.h:35`): `users` é um `GArray` global.
- `debug` é macro de `debug_level` (`bygfoot.h:97`).
- `window` é uma estrutura global de janelas GTK (`window.live` em `live_game.c:140`).

Onde essas variáveis são definidas e inicializadas **ainda não foi rastreado**. A Tarefa 5 (spike) precisa descobrir isso, porque o `bf-headless` terá que inicializar `status`, `users`, as definições de ligas/times e o gerador de números aleatórios sem passar por `main.c`.

### Pendências para a Tarefa 5

1. Ler `misc_callback_pause_live_game` e `treeview_show_game_stats` e confirmar que só são alcançados com usuário presente.
2. Localizar a inicialização de `status`, `users`, opções e do gerador aleatório.
3. Descobrir como a seed é aplicada (a regra do projeto exige mesma seed, mesmo resultado).
4. Medir quantos símbolos de interface o linker pede ao compilar só o núcleo.
