#!/bin/sh
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
A="$ROOT/tools/apply-patches.sh"
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
fail() { echo "FALHA: $1"; exit 1; }
mk() { # monta upstream e patches sintéticos em $T/u e $T/p
  rm -rf "$T/u" "$T/p" "$T/d"; mkdir -p "$T/u/src" "$T/p"
  printf 'a\nb\nc\n' > "$T/u/src/f.c"
  printf '| arquivo.patch | arquivo do upstream | motivo | candidato nº (docs/spikes/headless-result.md) |\n|---|---|---|---|\n' > "$T/p/MANIFEST.md"
}
good_patch() { cat > "$T/p/0001-x.patch" <<'P'
--- a/src/f.c
+++ b/src/f.c
@@ -1,3 +1,3 @@
 a
-b
+B
 c
P
}
run() { UPSTREAM_DIR="$T/u" PATCH_DIR="$T/p" sh "$A" "$T/d" > "$T/o" 2>&1; }

mk; good_patch; echo '| 0001-x.patch | src/f.c | teste | 1 |' >> "$T/p/MANIFEST.md"
run; [ $? = 0 ] || fail "aplicar: esperado 0 ($(cat "$T/o"))"
[ "$(sed -n 2p "$T/d/src/f.c")" = B ] || fail "remendo não alterou a cópia"
[ "$(sed -n 2p "$T/u/src/f.c")" = b ] || fail "o upstream sintético foi alterado"

mk; good_patch; echo '| 0001-x.patch | src/f.c | teste | 1 |' >> "$T/p/MANIFEST.md"
printf 'a\nX\nc\n' > "$T/u/src/f.c"   # contexto mudou
run; [ $? = 1 ] || fail "contexto errado: esperado 1"
[ ! -e "$T/d" ] || fail "contexto errado: DESTINO deveria ter sido removido"

mk; good_patch   # .patch fora do manifesto
run; [ $? = 1 ] || fail "patch fora do manifesto: esperado 1"
[ ! -e "$T/d" ] || fail "patch fora do manifesto: DESTINO deveria ter sido removido"

mk; echo '| 0009-nao-existe.patch | src/f.c | teste | 1 |' >> "$T/p/MANIFEST.md"
run; [ $? = 1 ] || fail "manifesto com arquivo inexistente: esperado 1"
[ ! -e "$T/d" ] || fail "manifesto inexistente: DESTINO deveria ter sido removido"

mk; mkdir "$T/d"
run; [ $? = 2 ] || fail "DESTINO existente: esperado 2"
[ -d "$T/d" ] || fail "DESTINO existente não pode ser apagado"

mk; run; [ $? = 0 ] || fail "sem remendos: esperado 0"
diff -r "$T/u" "$T/d" > /dev/null || fail "sem remendos: cópia deve ser idêntica"
echo "OK"
