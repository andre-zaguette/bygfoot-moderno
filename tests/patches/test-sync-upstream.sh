#!/bin/sh
# LENTO (compila o upstream; precisa de rede e do remoto "upstream"). Rode com: sh tests/patches/test-sync-upstream.sh
# Num clone temporário: troca upstream/ pela tag 3.0.0 e sincroniza com o commit fixado.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
PIN=$(sed -n 's/^commit //p' "$ROOT/tools/upstream-pin.txt")
BASE=767acfa30a45e7fa091e14745cf53761482cb9d2
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
fail() { echo "FALHA: $1"; exit 1; }

# 1) sem argumento ou com commit curto: exit 2 e nada alterado
sh "$ROOT/tools/sync-upstream.sh" > "$T/o" 2>&1; [ $? = 2 ] || fail "sem argumento: esperado exit 2"
sh "$ROOT/tools/sync-upstream.sh" 9b7b0062 > "$T/o" 2>&1; [ $? = 2 ] || fail "commit curto: esperado exit 2"

# 2) clone limpo com upstream/ na 3.0.0
git clone -q --no-hardlinks "$ROOT" "$T/c" || fail "clone"
cd "$T/c" || exit 2
git config user.name t; git config user.email t@example.com
git remote add upstream "$(git -C "$ROOT" remote get-url upstream)"
git rm -r -q upstream && git commit -q -m "tira upstream" || fail "git rm"
git subtree add -q --prefix=upstream upstream "$BASE" --squash || fail "subtree add da 3.0.0"
printf 'commit %s\ntree %s\n' "$BASE" "$(git rev-parse "$BASE^{tree}")" > tools/upstream-pin.txt
git add -A && git commit -q -m "fixa 3.0.0" || fail "commit da base"

# 3) árvore suja: recusa com exit 1
echo x > sujeira.txt
sh tools/sync-upstream.sh "$PIN" > "$T/o" 2>&1; [ $? = 1 ] || fail "árvore suja: esperado exit 1"
rm sujeira.txt

# 4) sincroniza com o commit fixado do projeto
sh tools/sync-upstream.sh "$PIN" > "$T/o" 2>&1; rc=$?
tail -5 "$T/o"
[ $rc = 0 ] || fail "sync: esperado exit 0, veio $rc"
grep -q "^SEEDS_IGUAIS$\|^SEEDS_MUDARAM " "$T/o" || fail "saída sem SEEDS_IGUAIS/SEEDS_MUDARAM"
grep -q "^commit $PIN$" tools/upstream-pin.txt || fail "upstream-pin.txt não aponta o novo commit"
sh tools/verify-upstream-pristine.sh "$PIN" > /dev/null || fail "upstream/ não está intacto após o sync"
echo "OK"
