#!/bin/sh
# Canário: documenta o defeito do upstream (JSON inválido sob locale com vírgula decimal).
# Sai com 0 enquanto o defeito EXISTE. Se sair com 1, o upstream corrigiu e o wrapper pode relaxar.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BIN=$ROOT/build/upstream/bygfoot
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
mkdir -p "$T/home"; ln -s "$ROOT/upstream/support_files" "$T/support_files"
cat > "$T/c.json" <<'JSON'
{ "commands" : [ { "add_country" : { "name" : "faroe_islands" }}, { "start_bygfoot" : { }}, { "dump_bygfoot" : { }} ] }
JSON
(cd "$T" && env -u DISPLAY LC_ALL=pt_BR.UTF-8 HOME="$T/home" "$BIN" --json c.json --random-seed 1 > out.json 2>/dev/null)
if python3 -I -c 'import json,sys; json.load(open(sys.argv[1],encoding="utf-8"))' "$T/out.json" 2>/dev/null; then
  echo "O defeito NÃO reproduziu: JSON válido sob pt_BR. Reavalie o LC_ALL=C do wrapper."; exit 1
fi
echo "OK: defeito reproduzido (JSON inválido sob pt_BR.UTF-8)"
