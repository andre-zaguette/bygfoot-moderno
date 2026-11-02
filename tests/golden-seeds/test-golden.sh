#!/bin/sh
# Compara os hashes das seeds de ouro com tests/golden-seeds/expected-macos.txt.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
EXP="$ROOT/tests/golden-seeds/expected-macos.txt"
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
[ -f "$EXP" ] || { echo "FALHA: falta $EXP"; exit 1; }
python3 -I "$ROOT/tools/golden-hash.py" > "$T/got.txt" || { echo "FALHA: golden-hash.py saiu com erro"; exit 1; }
if ! cmp -s "$EXP" "$T/got.txt"; then
  echo "FALHA: hashes diferem do esperado; primeira linha divergente:"
  diff "$EXP" "$T/got.txt" | sed -n '1,4p'
  exit 1
fi
echo "OK"
