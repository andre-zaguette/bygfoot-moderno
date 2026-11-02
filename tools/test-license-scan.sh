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

# Amostras sintéticas: cada classe deve ser reconhecida pelo texto do cabeçalho.
S=$(mktemp -d); trap 'rm -rf "$S"' EXIT
mkdir -p "$S/src" "$S/support_files/arte"
hdr() { printf '/*\n   %s\n*/\nint x;\n' "$2" > "$S/src/$1"; }
hdr or_later.c  "This program is free software; you can redistribute it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version."
hdr only2.c     "This program is free software under the GNU General Public License, version 2 only."
hdr lib.c       "Licensed under the GNU Lesser General Public License, version 2.1."
hdr v3.c        "Licensed under the GNU General Public License as published by the FSF, version 3."
hdr none.c      "Copyright 2026 Fulano. Todos os direitos reservados."
printf 'x' > "$S/support_files/arte/a.png"; printf 'x' > "$S/support_files/arte/LICENSE.txt"
o=$(sh tools/license-scan.sh "$S")
chk() { echo "$o" | grep -qx "FONTE | $1 | $2" || { echo "FALHA: $1 deveria ser $2. Saída:"; echo "$o"; exit 1; }; }
chk or_later.c GPL-2.0-or-later
chk only2.c    GPL-2.0-only
chk lib.c      LGPL
chk v3.c       GPL-OUTRA
chk none.c     SEM-CABECALHO
echo "$o" | grep -q '^ASSET | arte | 2 | .*LICENSE.txt' || { echo "FALHA: nota de licença do asset não reconhecida"; echo "$o"; exit 1; }
echo "OK"
