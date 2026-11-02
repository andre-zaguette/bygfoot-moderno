#!/bin/sh
# apply-patches.sh DESTINO — copia upstream/ para DESTINO e aplica patches/*.patch em ordem.
# Nunca altera upstream/. Em qualquer erro remove DESTINO. Saída: 0 ok | 1 remendo/manifesto inválido | 2 uso/DESTINO existe.
ROOT=$(cd "$(dirname "$0")/.." && pwd)
UP=${UPSTREAM_DIR:-$ROOT/upstream}
PD=${PATCH_DIR:-$ROOT/patches}
DEST=$1
[ -n "$DEST" ] || { echo "uso: apply-patches.sh DESTINO" >&2; exit 2; }
[ ! -e "$DEST" ] || { echo "apply-patches: DESTINO já existe: $DEST" >&2; exit 2; }
bad() { echo "apply-patches: $1" >&2; rm -rf "$DEST"; exit 1; }

[ -f "$PD/MANIFEST.md" ] || bad "falta $PD/MANIFEST.md"
# toda linha de tabela do manifesto cuja 1ª coluna termina em .patch deve existir
listed=$(sed -n 's/^| *\([0-9][^ |]*\.patch\) *|.*/\1/p' "$PD/MANIFEST.md")
for p in $listed; do [ -f "$PD/$p" ] || bad "o MANIFEST lista $p, que não existe"; done
# todo .patch da pasta deve estar no manifesto
for f in "$PD"/*.patch; do
  [ -e "$f" ] || continue
  b=$(basename "$f")
  echo "$listed" | grep -qx "$b" || bad "$b não está no MANIFEST.md"
done

cp -R "$UP" "$DEST" || bad "falha ao copiar $UP"
# 1º passa tudo em seco (a cópia só é alterada se todos encaixarem); como remendos podem depender uns dos outros, o seco é feito numa cópia descartável
DRY=$(mktemp -d) || bad "sem diretório temporário"
cp -R "$UP/." "$DRY/" || { rm -rf "$DRY"; bad "falha ao copiar para teste a seco"; }
for f in "$PD"/*.patch; do
  [ -e "$f" ] || continue
  (cd "$DRY" && patch -p1 -s < "$f") >/dev/null 2>&1 || { rm -rf "$DRY"; bad "remendo não encaixa: $(basename "$f")"; }
done
rm -rf "$DRY"
for f in "$PD"/*.patch; do
  [ -e "$f" ] || continue
  (cd "$DEST" && patch -p1 -s < "$f") >/dev/null 2>&1 || bad "falha ao aplicar $(basename "$f")"
done
exit 0
