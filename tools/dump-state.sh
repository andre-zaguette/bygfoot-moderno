#!/bin/sh
# dump-state.sh SEED — imprime o digest (caminho<TAB>hash) do estado inicial, logo após start_bygfoot, sem simular.
# Variáveis: BYGFOOT_BIN, BYGFOOT_SUPPORT. Diagnóstico de determinismo (ADR-002).
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/.." && pwd)
BIN=${BYGFOOT_BIN:-$ROOT/build/upstream/bygfoot}
SUPPORT=${BYGFOOT_SUPPORT:-$ROOT/upstream/support_files}
seed=$1
case $seed in ''|*[!0-9]*) echo "uso: dump-state.sh SEED" >&2; exit 2 ;; esac
T=$(mktemp -d) || exit 3
trap 'rm -rf "$T"' EXIT INT TERM
mkdir -p "$T/home"; ln -s "$SUPPORT" "$T/support_files"
cat > "$T/cmds.json" <<JSON
{ "commands" : [
  { "add_user" : { "username" : "spike", "country" : "faroe_islands", "team" : "V. Gotu" }},
  { "start_bygfoot" : { }},
  { "dump_bygfoot" : { }}
]}
JSON
(cd "$T" && env -u DISPLAY LC_ALL=C LANG=C HOME="$T/home" "$BIN" --json "$T/cmds.json" --random-seed "$seed" > "$T/out.json" 2> "$T/err.txt")
[ -s "$T/out.json" ] || { [ -s "$T/stdout.log" ] && cp "$T/stdout.log" "$T/out.json"; }
[ -s "$T/out.json" ] || { echo "dump-state: sem saída" >&2; head -c 1000 "$T/err.txt" >&2; exit 3; }
[ -z "$STATE_SAMPLE" ] || { python3 -I "$HERE/state-sample.py" "$T/out.json"; exit $?; }
python3 -I "$HERE/state-digest.py" "$T/out.json" "${DIGEST_DEPTH:-6}"
