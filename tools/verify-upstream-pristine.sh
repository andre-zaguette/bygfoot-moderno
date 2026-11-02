#!/bin/sh
# Uso: verify-upstream-pristine.sh <commit-do-upstream>
# Sai com 0 se upstream/ é idêntico (na árvore commitada e no disco) ao commit informado.
cd "$(dirname "$0")/.." || exit 2
commit=$1
[ -n "$commit" ] || { echo "uso: $0 <commit>"; exit 2; }
[ -d upstream ] || { echo "FALHA: upstream/ ausente"; exit 1; }
want=$(git rev-parse --verify -q "$commit^{tree}")
if [ -z "$want" ]; then
  # Sem o objeto do commit (clone raso): usa a árvore fixada em tools/upstream-pin.txt.
  pc=$(sed -n 's/^commit //p' tools/upstream-pin.txt 2>/dev/null)
  case "$pc" in
    "$commit"*) want=$(sed -n 's/^tree //p' tools/upstream-pin.txt) ;;
  esac
fi
[ -n "$want" ] || { echo "FALHA: commit $commit desconhecido e sem entrada em tools/upstream-pin.txt (git fetch upstream)"; exit 1; }
have=$(git rev-parse "HEAD:upstream") || exit 1
rc=0
if [ "$want" != "$have" ]; then
  echo "FALHA: upstream/ difere do commit $commit:"
  git diff-tree -r --name-only "$want" "$have"
  rc=1
fi
dirty=$(git status --porcelain --untracked-files=all -- upstream)
if [ -n "$dirty" ]; then echo "FALHA: alterações no disco em upstream/:"; echo "$dirty"; rc=1; fi
[ "$rc" -eq 0 ] && echo "OK: upstream/ idêntico a $commit"
exit "$rc"
