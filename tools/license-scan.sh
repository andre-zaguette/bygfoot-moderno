#!/bin/sh
# Uso: license-scan.sh <pasta-upstream>
# FONTE | arquivo | classe  (lida nos primeiros 40 linhas de cada .c/.h de src/)
# RESUMO | contagem por classe
# ASSET | pasta | arquivos | extensões | notas de licença encontradas
# Classes: GPL-2.0-or-later, GPL-2.0-only, GPL-OUTRA, LGPL, SEM-CABECALHO.
# Só reporta fatos do texto; não conclui que algo é permitido.
root=${1:?uso: $0 <pasta-upstream>}
src="$root/src"
tmp=$(mktemp) || exit 2
ls "$src" | LC_ALL=C sort | while read -r f; do
  case "$f" in *.c|*.h) ;; *) continue ;; esac
  txt=$(head -40 "$src/$f" | tr '\n' ' ' | tr -s ' *')
  case "$txt" in
    *"Lesser General Public"*) c=LGPL ;;
    *"General Public License"*"version 2"*"any later version"*) c=GPL-2.0-or-later ;;
    *"General Public License"*"version 2"*) c=GPL-2.0-only ;;
    *"General Public License"*) c=GPL-OUTRA ;;
    *) c=SEM-CABECALHO ;;
  esac
  echo "FONTE | $f | $c"
done > "$tmp"
cat "$tmp"
printf 'RESUMO | '
awk -F' \\| ' '{n[$3]++} END{first=1; for(k in n){keys[++i]=k}; for(a=1;a<=i;a++)for(b=a+1;b<=i;b++)if(keys[b]<keys[a]){t=keys[a];keys[a]=keys[b];keys[b]=t}; for(a=1;a<=i;a++){printf "%s%s=%d", (a>1?", ":""), keys[a], n[keys[a]]}; print ""}' "$tmp"
rm -f "$tmp"
sf="$root/support_files"
ls "$sf" | LC_ALL=C sort | while read -r d; do
  [ -d "$sf/$d" ] || continue
  n=$(find "$sf/$d" -type f | wc -l | tr -d ' ')
  ext=$(find "$sf/$d" -type f | sed 's/.*\///' | awk -F. '{print (NF>1?$NF:"(sem)")}' | LC_ALL=C sort | uniq -c | awk '{printf "%s%s:%s", (NR>1?",":""), $2, $1}')
  notes=$(find "$sf/$d" -type f \( -iname '*licen*' -o -iname '*copyright*' -o -iname 'readme*' -o -iname 'authors*' \) | sed "s|$sf/||" | LC_ALL=C sort | tr '\n' ' ')
  echo "ASSET | $d | $n | $ext | ${notes:-nenhuma}"
done
