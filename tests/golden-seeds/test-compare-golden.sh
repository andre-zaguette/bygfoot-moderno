#!/bin/sh
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
C="$ROOT/tools/compare-golden.py"
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
fail() { echo "FALHA: $1"; exit 1; }
printf '1\tA\t2\taaa\t10\n2\tB\t2\tbbb\t11\n' > "$T/a"
cp "$T/a" "$T/b"; cp "$T/a" "$T/c"
python3 -I "$C" "$T/a" "$T/b" "$T/c" > "$T/o" 2>&1; rc=$?
[ "$rc" = 0 ] || fail "iguais: esperado 0, veio $rc"
printf '1\tA\t2\taaa\t10\n2\tB\t2\tXXX\t11\n' > "$T/c"
python3 -I "$C" "$T/a" "$T/b" "$T/c" > "$T/o" 2>&1; rc=$?
[ "$rc" = 1 ] || fail "hash diferente: esperado 1, veio $rc"
grep -q "2" "$T/o" && grep -q "XXX" "$T/o" && grep -q "bbb" "$T/o" || fail "saída não mostra a seed e os hashes"
grep -q "aaa" "$T/o" && fail "seed igual não deve ser listada"
# diferença só de fim de linha (CRLF) não pode passar por igual
printf '1\tA\t2\taaa\t10\r\n2\tB\t2\tbbb\t11\r\n' > "$T/c"
python3 -I "$C" "$T/a" "$T/b" "$T/c" > "$T/o" 2>&1; rc=$?
[ "$rc" = 1 ] || fail "CRLF: esperado 1, veio $rc"
python3 -I "$C" "$T/a" "$T/nao-existe" > "$T/o" 2>&1; rc=$?
[ "$rc" = 2 ] || fail "arquivo ausente: esperado 2, veio $rc"
# seed presente num arquivo e ausente em outro
printf '1\tA\t2\taaa\t10\n' > "$T/c"
python3 -I "$C" "$T/a" "$T/c" > "$T/o" 2>&1; rc=$?
[ "$rc" = 1 ] || fail "linha faltando: esperado 1, veio $rc"
echo "OK"
