#!/bin/sh
# Teste de tools/sqlite-schema.sh (valores lidos de upstream/src/sqlite_data_store.c).
cd "$(dirname "$0")/.." || exit 2
a=$(sh tools/sqlite-schema.sh upstream/src) || { echo "FALHA: script não rodou"; exit 1; }
b=$(sh tools/sqlite-schema.sh upstream/src)
[ "$a" = "$b" ] || { echo "FALHA: saída instável"; exit 1; }
n=$(echo "$a" | grep -c 'CREATE TABLE'); [ "$n" = "14" ] || { echo "FALHA: $n tabelas (esperado 14)"; exit 1; }
i=$(echo "$a" | grep -c 'CREATE INDEX'); [ "$i" = "2" ] || { echo "FALHA: $i índices (esperado 2)"; exit 1; }
echo "$a" | grep -q 'CREATE TABLE player (' || { echo "FALHA: tabela player ausente"; exit 1; }
echo "$a" | grep -q 'CREATE TABLE league (' || { echo "FALHA: tabela league ausente"; exit 1; }
echo "OK"
