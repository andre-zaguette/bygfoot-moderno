#!/bin/sh
# sync-upstream.sh COMMIT_COMPLETO — traz um commit do upstream para upstream/ (git subtree pull --squash),
# atualiza tools/upstream-pin.txt, confere que upstream/ está intacto, aplica patches/ numa cópia, compila e
# roda as seeds de ouro. Imprime SEEDS_IGUAIS ou "SEEDS_MUDARAM <seeds>". A decisão de aceitar é humana.
# Saída: 0 (os dois resultados) | 1 falha de árvore suja, merge, remendo ou build | 2 uso.
# Variável: UPSTREAM_REMOTE (padrão: upstream).
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
REMOTE=${UPSTREAM_REMOTE:-upstream}
C=$1
case $C in [0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]*) ;; *) echo "uso: sync-upstream.sh COMMIT_COMPLETO (40 hex)" >&2; exit 2 ;; esac
[ ${#C} -eq 40 ] || { echo "uso: sync-upstream.sh COMMIT_COMPLETO (40 hex)" >&2; exit 2; }
case $C in *[!0-9a-f]*) echo "commit inválido: $C" >&2; exit 2 ;; esac
die() { echo "sync-upstream: $1" >&2; exit 1; }

[ -z "$(git status --porcelain)" ] || die "a árvore de trabalho não está limpa"
git fetch -q "$REMOTE" "$C" || die "não consegui buscar $C em $REMOTE"
git rev-parse -q --verify "$C^{commit}" > /dev/null || die "commit $C não existe após o fetch"

git subtree pull -q --prefix=upstream "$REMOTE" "$C" --squash -m "chore(upstream): sincroniza com $C" || die "git subtree pull falhou (conflito?)"
printf '%s\ncommit %s\ntree %s\n' "$(sed -n '1p' tools/upstream-pin.txt)" "$C" "$(git rev-parse "$C^{tree}")" > tools/upstream-pin.txt
git add tools/upstream-pin.txt && git commit -q -m "chore(upstream): fixa $C" || die "não consegui registrar o novo pin"
sh tools/verify-upstream-pristine.sh "$C" > /dev/null || die "upstream/ não é idêntico ao commit $C"

DEST=$ROOT/build/sync-src
rm -rf "$DEST" "$ROOT/build/sync"
sh tools/apply-patches.sh "$DEST" || die "algum remendo não encaixa no novo upstream (veja patches/MANIFEST.md)"
BF_SRC=build/sync-src BF_BUILD=build/sync sh tools/build-upstream.sh build/sync-build.log > /dev/null || die "o build do novo upstream falhou (build/sync-build.log)"

OUT=$(mktemp) || die "sem arquivo temporário"
BYGFOOT_BIN="$ROOT/build/sync/bygfoot" BYGFOOT_SUPPORT="$DEST/support_files" python3 -I tools/golden-hash.py > "$OUT" || { rm -f "$OUT"; die "as seeds de ouro falharam no novo upstream"; }
if cmp -s "$OUT" tests/golden-seeds/expected-patched.txt; then
  echo SEEDS_IGUAIS
else
  CH=$(awk -F'\t' 'NR==FNR{e[$1"|"$2"|"$3]=$4; next} (e[$1"|"$2"|"$3]!=$4){printf "%s ", $1}' tests/golden-seeds/expected-patched.txt "$OUT")
  echo "SEEDS_MUDARAM $CH"
fi
rm -f "$OUT"
exit 0
