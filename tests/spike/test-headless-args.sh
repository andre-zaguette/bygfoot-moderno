#!/bin/sh
# Validação de argumentos do bf-headless. Nenhum caso chega a rodar o motor, então é rápido.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BF="$ROOT/spike/headless/bf-headless"
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
fail() { echo "FALHA: $1"; exit 1; }

# roda "$@" com limite de 5 s; devolve o código de saída (143 = travou e foi morto)
run5() {
  "$BF" "$@" > /dev/null 2> "$T/err" &
  pid=$!
  ( sleep 5; kill "$pid" 2>/dev/null ) &
  killer=$!
  wait "$pid"; rc=$?
  kill "$killer" 2>/dev/null; wait "$killer" 2>/dev/null
  return $rc
}
expect2() { # descrição, args...
  d=$1; shift
  run5 "$@"; rc=$?
  [ "$rc" = "2" ] || fail "$d: esperado exit 2, veio $rc"
  [ -s "$T/err" ] || fail "$d: exit 2 sem mensagem em stderr"
}

expect2 "--team sem valor"      --seed 1 --team
expect2 "--seed sem valor"      --seed
expect2 "--weeks sem valor"     --seed 1 --weeks
expect2 "--country sem valor"   --seed 1 --country
expect2 "aspas em --team"       --seed 1 --team 'V. "Gotu'
expect2 "barra em --country"    --seed 1 --country 'faroe\islands'
expect2 "quebra de linha em --team" --seed 1 --team "$(printf 'a\nb')"
expect2 "argumento desconhecido" --seed 1 --nada

# Time inexistente: o motor cai; o wrapper sai 3, sem nada no stdout e com mensagem clara.
"$BF" --seed 1 --team "Time Inexistente" > "$T/out" 2> "$T/err"; rc=$?
[ "$rc" = "3" ] || fail "time inexistente: esperado exit 3, veio $rc"
[ ! -s "$T/out" ] || fail "time inexistente: stdout deveria estar vazio"
grep -q "time" "$T/err" || fail "time inexistente: stderr sem a palavra 'time'"
# --weeks acima do teto: recusado com exit 2 citando o teto, sem rodar o motor.
run5 --seed 1 --weeks 300; rc=$?
[ "$rc" = "2" ] || fail "--weeks 300: esperado exit 2, veio $rc"
grep -q "teto" "$T/err" || fail "--weeks 300: mensagem não cita o teto"
echo "OK"
