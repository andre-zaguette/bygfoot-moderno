#!/bin/sh
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
F="$ROOT/tools/first-divergence.py"
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
fail() { echo "FALHA: $1"; exit 1; }
printf 'l1\nl2\nl3\nl4\n' > "$T/a"; cp "$T/a" "$T/b"
python3 -I "$F" "$T/a" "$T/b" > "$T/o"; [ $? = 0 ] || fail "iguais: esperado 0"
printf 'l1\nl2\nXX\nl4\n' > "$T/b"
python3 -I "$F" "$T/a" "$T/b" > "$T/o"; rc=$?
[ $rc = 1 ] || fail "diferentes: esperado 1, veio $rc"
grep -q "3" "$T/o" && grep -q "l3" "$T/o" && grep -q "XX" "$T/o" || fail "saída deve mostrar índice 3 e os dois conteúdos"
printf 'l1\nl2\n' > "$T/b"
python3 -I "$F" "$T/a" "$T/b" > "$T/o"; [ $? = 1 ] || fail "tamanho diferente: esperado 1"
echo "OK"
