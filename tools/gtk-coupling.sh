#!/bin/sh
# Uso: gtk-coupling.sh <pasta-src>
# Tabela (ordem estável) de acoplamento de cada .c/.h com a interface gráfica.
# Colunas: arquivo | gtk (linhas que usam a API: gtk_, GTK_, Gtk*, gdk_, Gdk*) | gui-includes
# | params GUI*gui (linhas com "GUI *gui") | extern (linhas que começam com "extern")
dir=${1:?uso: $0 <pasta-src>}
echo "arquivo | gtk | gui-includes | params GUI*gui | extern"
ls "$dir" | LC_ALL=C sort | while read -r f; do
  case "$f" in *.c|*.h) ;; *) continue ;; esac
  p="$dir/$f"
  gtk=$(grep -cE '\b(gtk_|GTK_|Gtk[A-Z]|gdk_|Gdk[A-Z])' "$p")
  inc=$(grep -cE '#include.*(gui|callbacks|window|treeview)' "$p")
  par=$(grep -c 'GUI \*gui' "$p")
  ext=$(grep -c '^extern' "$p")
  echo "$f | $gtk | $inc | $par | $ext"
done
