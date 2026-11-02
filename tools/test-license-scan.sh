#!/bin/sh
# Teste de tools/license-scan.sh sobre o upstream fixado.
cd "$(dirname "$0")/.." || exit 2
a=$(sh tools/license-scan.sh upstream) || { echo "FALHA: script não rodou"; exit 1; }
b=$(sh tools/license-scan.sh upstream)
[ "$a" = "$b" ] || { echo "FALHA: saída instável"; exit 1; }
n=$(echo "$a" | grep -c '^FONTE | '); [ "$n" = "201" ] || { echo "FALHA: $n linhas FONTE (esperado 201)"; exit 1; }
echo "$a" | grep -q '^FONTE | fixture.c | GPL-2.0-or-later$' || { echo "FALHA: fixture.c deveria ser GPL-2.0-or-later"; exit 1; }
echo "$a" | grep -q '^RESUMO | ' || { echo "FALHA: sem linha RESUMO"; exit 1; }
echo "$a" | grep -q '^ASSET | pixmaps | ' || { echo "FALHA: sem linha ASSET de pixmaps"; exit 1; }
echo "OK"
