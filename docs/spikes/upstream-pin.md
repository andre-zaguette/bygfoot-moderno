# Versão do upstream fixada

- Remote: `upstream` = https://gitlab.com/bygfoot/bygfoot.git (branch `main`)
- **Fixado: `9b7b0062`** ("Require CMake 3.14 or newer"), importado em `upstream/` com `git subtree add --squash`.
- Tag `3.0.0` = `767acfa`. É ancestral do commit fixado.

## Diferença entre a tag e o commit fixado

- 9b7b0062 Require CMake 3.14 or newer
- ed3b7cd7 Fix SQLite index result handling when loading saves
- 964e120d Port Bygfoot from GTK 2 to GTK 3
- f42365a8 Use version-neutral GTK wording
- 199b66bb Rename the GTK frontend enum value
- ee55f8a2 Optimize player_update_week_roundly()
- 26f44bc6 Refactor fixture_update_write_replays
- c25eb954 Replace query_league_cup_has_property() with faster alternative
- 1a02a95a Move properties member from Cup and League structs to Competition struct
- 7eeb3d2c Refactor fixture_get_cup_round_winners

`git diff --shortstat 767acfa 9b7b0062`:  63 files changed, 190 insertions(+), 192 deletions(-)

## Escolha e motivo

Fixei o commit à frente da tag. Ele traz o port de GTK 2 para GTK 3 e o build em CMake, o que facilita compilar em macOS, e uma correção no carregamento de saves SQLite. As mudanças de lógica entre a tag e este commit são refactors e otimizações pequenas (fixture, league, player), a validar pelo teste de seeds fixas da Fase 1.

Se o build da Tarefa 3 ou o spike da Tarefa 5 falharem por causa desses 10 commits, a alternativa é refixar na tag `3.0.0`. Custo: perde o port GTK 3.

## Verificação

`sh tools/verify-upstream-pristine.sh 9b7b0062` deve sair com 0 sempre que `upstream/` não tiver sido editado.
