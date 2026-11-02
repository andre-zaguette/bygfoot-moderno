#!/bin/sh
# Teste de tools/gtk-coupling.sh: estabilidade e valores medidos na leitura do upstream.
cd "$(dirname "$0")/.." || exit 2
a=$(sh tools/gtk-coupling.sh upstream/src) || { echo "FALHA: script não rodou"; exit 1; }
b=$(sh tools/gtk-coupling.sh upstream/src)
[ "$a" = "$b" ] || { echo "FALHA: saída instável entre execuções"; exit 1; }
col() { echo "$a" | awk -F' *\\| *' -v f="$1" -v c="$2" '$1==f{print $c}'; }
[ "$(col live_game.c 4)" = "13" ] || { echo "FALHA: live_game.c params GUI*gui = '$(col live_game.c 4)' (esperado 13)"; exit 1; }
[ "$(col fixture.c 3)" = "0" ]   || { echo "FALHA: fixture.c includes de gui = '$(col fixture.c 3)' (esperado 0)"; exit 1; }
[ "$(col fixture.c 2)" = "0" ]   || { echo "FALHA: fixture.c usos da API GTK = '$(col fixture.c 2)' (esperado 0)"; exit 1; }
[ "$(col live_game.c 2)" = "3" ]  || { echo "FALHA: live_game.c usos da API GTK = '$(col live_game.c 2)' (esperado 3)"; exit 1; }
echo "OK"
