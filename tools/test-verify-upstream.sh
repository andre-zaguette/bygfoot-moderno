#!/bin/sh
# Teste do verificador num clone raso (como no CI): não tem o commit do upstream, só o do projeto.
ROOT=$(cd "$(dirname "$0")/.." && pwd)
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
git clone -q --no-local --depth 1 "file://$ROOT" "$T/c" || { echo "FALHA: clone raso"; exit 1; }
cd "$T/c" || exit 2
git cat-file -e 9b7b0062ddb90a620865314dddf0c4b1c0a23660 2>/dev/null && { echo "FALHA: o clone raso não deveria ter o commit do upstream (teste sem sentido)"; exit 1; }
sh tools/verify-upstream-pristine.sh 9b7b0062 > "$T/ok.out" 2>&1 || { echo "FALHA: verificador falhou num clone raso intacto:"; cat "$T/ok.out"; exit 1; }
echo "// alterado" >> upstream/src/main.c
sh tools/verify-upstream-pristine.sh 9b7b0062 > "$T/bad.out" 2>&1 && { echo "FALHA: verificador NÃO detectou arquivo alterado"; exit 1; }
git checkout -q -- upstream
echo "x" > upstream/src/novo_arquivo.c
sh tools/verify-upstream-pristine.sh 9b7b0062 > "$T/bad2.out" 2>&1 && { echo "FALHA: verificador NÃO detectou arquivo novo"; exit 1; }
echo "OK"
