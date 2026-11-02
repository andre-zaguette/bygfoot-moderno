#!/bin/sh
# Testes do spike headless. Compara a saída byte a byte.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BF="$ROOT/spike/headless/bf-headless"
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
fail() { echo "FALHA: $1"; exit 1; }

# 1. mesma seed duas vezes
"$BF" --seed 431199 > "$T/a1" 2> "$T/e1" || fail "execução base saiu com erro: $(head -c 200 "$T/e1")"
"$BF" --seed 431199 > "$T/a2" || fail "segunda execução falhou"
cmp -s "$T/a1" "$T/a2" || fail "mesma seed gerou saídas diferentes"
[ "$(wc -l < "$T/a1")" -gt 50 ] || fail "poucas linhas de lances ($(wc -l < "$T/a1"))"

# 2. seeds de borda: 0 e 2147483647 repetem; 4294967295 é recusada com exit 2
for s in 0 2147483647; do
  "$BF" --seed $s > "$T/s1" || fail "seed $s deveria funcionar"
  "$BF" --seed $s > "$T/s2" || fail "seed $s deveria funcionar (2ª vez)"
  cmp -s "$T/s1" "$T/s2" || fail "seed $s não repete"
done
"$BF" --seed 4294967295 > /dev/null 2> "$T/big.err"; rc=$?
[ "$rc" = "2" ] || fail "seed 4294967295 deveria sair com 2 (saiu com $rc)"
[ -s "$T/big.err" ] || fail "seed fora do intervalo deveria explicar o erro em stderr"

# 3. pasta de execução diferente (com espaço e acento) e raiz
D="$T/dir com espaço é"; mkdir -p "$D"
(cd "$D" && "$BF" --seed 431199 > "$T/c1") || fail "falhou em pasta com espaço/acento"
cmp -s "$T/a1" "$T/c1" || fail "saída mudou ao rodar de pasta com espaço/acento"
(cd / && "$BF" --seed 431199 > "$T/c2") || fail "falhou rodando de /"
cmp -s "$T/a1" "$T/c2" || fail "saída mudou ao rodar de /"

# 4. locale
LC_ALL=pt_BR.UTF-8 "$BF" --seed 431199 > "$T/l1" || fail "falhou com pt_BR"
LC_ALL=C "$BF" --seed 431199 > "$T/l2" || fail "falhou com C"
cmp -s "$T/a1" "$T/l1" && cmp -s "$T/a1" "$T/l2" || fail "saída depende do locale"

# 5. duas execuções simultâneas
"$BF" --seed 431199 > "$T/p1" & p1=$!
"$BF" --seed 431199 > "$T/p2" & p2=$!
wait $p1 || fail "execução paralela 1 falhou"; wait $p2 || fail "execução paralela 2 falhou"
cmp -s "$T/a1" "$T/p1" && cmp -s "$T/a1" "$T/p2" || fail "execuções simultâneas divergiram"

# 7. comportamento do Windows: o upstream redireciona stdout/stderr para stdout.log/stderr.log na pasta atual
cat > "$T/fake-win-bygfoot" <<FAKE
#!/bin/sh
exec "$ROOT/build/upstream/bygfoot" "\$@" > stdout.log 2> stderr.log
FAKE
chmod +x "$T/fake-win-bygfoot"
BYGFOOT_BIN="$T/fake-win-bygfoot" "$BF" --seed 431199 > "$T/w1" 2> "$T/w1.err" || fail "falhou com binário que escreve em stdout.log: $(head -c 200 "$T/w1.err")"
cmp -s "$T/a1" "$T/w1" || fail "saída mudou quando o motor escreve em stdout.log"

# 6. gols nos eventos somam o placar final
python3 -I - "$T/a1" <<'PY' || fail "gols dos eventos != placar final"
import json,sys
rows=[json.loads(l) for l in open(sys.argv[1],encoding="utf-8")]
final=[r for r in rows if "final" in r]; units=[r for r in rows if "final" not in r]
assert len(final)==1, "deve haver exatamente uma linha final"
goals=sum(1 for u in units if u["event"] in (10,11))
assert goals==sum(final[0]["final"]), f"{goals} gols nos eventos, placar {final[0]['final']}"
assert units[-1]["result"]==final[0]["final"], "último lance != placar final"
PY
echo "OK"
