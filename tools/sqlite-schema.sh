#!/bin/sh
# Uso: sqlite-schema.sh <pasta-src>
# Limitação conhecida: um CREATE entre aspas dentro de comentário também é listado (não ocorre no upstream fixado).
# Lista, em ordem estável, cada CREATE TABLE/INDEX/VIEW escrito no código C, como "arquivo:linha: SQL".
dir=${1:?uso: $0 <pasta-src>}
ls "$dir" | LC_ALL=C sort | while read -r f; do
  case "$f" in *.c) ;; *) continue ;; esac
  awk -v F="$f" '
    !on && /"CREATE[ \t]+(UNIQUE[ \t]+)?(TABLE|INDEX|VIEW)/ { on=1; start=NR; buf="" }
    on { line=$0; sub(/^[^"]*"/, "", line); sub(/"[^"]*$/, "", line); buf = buf line
         if ($0 ~ /;"/) { print F ":" start ": " buf; on=0 } }
  ' "$dir/$f"
done
