#!/bin/sh
# Compila o Bygfoot original (upstream/) fora da árvore, em build/upstream.
# Uso: build-upstream.sh [arquivo-de-log]   (padrão: docs/spikes/build-macos.log)
# Variáveis: BF_SRC (fonte, padrão upstream) e BF_BUILD (saída, padrão build/upstream).
cd "$(dirname "$0")/.." || exit 2
log=${1:-docs/spikes/build-macos.log}
mkdir -p "$(dirname "$log")"
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/opt/homebrew/share/pkgconfig:$PKG_CONFIG_PATH"
{
  echo "== $(date '+%Y-%m-%d %H:%M:%S') cmake $(cmake --version | head -1) | $(cc --version | head -1)"
  cmake -S "${BF_SRC:-upstream}" -B "${BF_BUILD:-build/upstream}" && cmake --build "${BF_BUILD:-build/upstream}" -j 4
  echo "== exit=$?"
} > "$log" 2>&1
rc=$(tail -1 "$log" | sed 's/.*exit=//')
tail -n 15 "$log"
exit "${rc:-1}"
