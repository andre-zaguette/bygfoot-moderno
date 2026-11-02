#!/bin/sh
# Teste de tools/sqlite-schema.sh (valores lidos de upstream/src/sqlite_data_store.c).
cd "$(dirname "$0")/.." || exit 2
a=$(sh tools/sqlite-schema.sh upstream/src) || { echo "FALHA: script não rodou"; exit 1; }
b=$(sh tools/sqlite-schema.sh upstream/src)
[ "$a" = "$b" ] || { echo "FALHA: saída instável"; exit 1; }
n=$(echo "$a" | grep -c 'CREATE TABLE'); [ "$n" = "14" ] || { echo "FALHA: $n tabelas (esperado 14)"; exit 1; }
i=$(echo "$a" | grep -c 'CREATE INDEX'); [ "$i" = "2" ] || { echo "FALHA: $i índices (esperado 2)"; exit 1; }
echo "$a" | grep -q 'CREATE TABLE player (' || { echo "FALHA: tabela player ausente"; exit 1; }
echo "$a" | grep -q 'CREATE TABLE league (' || { echo "FALHA: tabela league ausente"; exit 1; }

# Amostra sintética: instrução em várias linhas, INSERT que não conta e arquivo sem CREATE.
S=$(mktemp -d); trap 'rm -rf "$S"' EXIT
cat > "$S/a.c" <<'C'
/* comentário sem SQL */
static const char *sql[] = {
    "CREATE TABLE t1 (id INTEGER PRIMARY KEY, "
    "name TEXT);",
    "INSERT INTO t1 VALUES (1, 'x');",
    "CREATE INDEX IF NOT EXISTS t1_name ON t1(name);",
};
C
printf 'int main(void){return 0;}\n' > "$S/b.c"
o=$(sh tools/sqlite-schema.sh "$S")
echo "$o" | grep -qx 'a.c:3: CREATE TABLE t1 (id INTEGER PRIMARY KEY, name TEXT);' || { echo "FALHA: CREATE de várias linhas não foi juntado. Saída:"; echo "$o"; exit 1; }
echo "$o" | grep -qx 'a.c:6: CREATE INDEX IF NOT EXISTS t1_name ON t1(name);' || { echo "FALHA: índice não extraído. Saída:"; echo "$o"; exit 1; }
echo "$o" | grep -q 'INSERT' && { echo "FALHA: INSERT não deveria aparecer"; exit 1; }
echo "$o" | grep -q '^b.c' && { echo "FALHA: arquivo sem CREATE apareceu"; exit 1; }
echo "OK"
