#!/bin/sh
# Verifica se CLAUDE.md e o roadmap refletem as decisões do projeto.
cd "$(dirname "$0")/.." || exit 2
fail=0
chk() { # descricao arquivo regex
  if grep -Eq "$3" "$2"; then :; else echo "FALTA: $1 ($2)"; fail=1; fi
}
chk "macOS no MVP (coluna Must de Distribuição)" ROADMAP_BYGFOOT_MODERNO.md '^\| Distribuição \| [^|]*macOS'
chk "patches/MANIFEST.md"                         CLAUDE.md                  'patches/MANIFEST\.md'
chk "camada de coreografia"                       ROADMAP_BYGFOOT_MODERNO.md 'coreografia'
chk "Caminho A como principal"                    ROADMAP_BYGFOOT_MODERNO.md 'Caminho A.*principal'
chk "Caminho A como principal"                    CLAUDE.md                  'Caminho A.*principal'
[ "$fail" -eq 0 ] && echo OK
exit "$fail"
