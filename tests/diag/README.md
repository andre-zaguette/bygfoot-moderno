# Remendos de diagnóstico (NÃO fazem parte do produto)

Usados na Fase 1 para localizar a divergência de determinismo entre sistemas (ADR-002). Não estão em `patches/` nem no `MANIFEST.md`, e `tools/apply-patches.sh` os recusaria de propósito.

Aplicar sobre uma cópia já remendada, nesta ordem:

```
sh tools/apply-patches.sh build/patched-src
cp -R build/patched-src build/trace-src
cd build/trace-src
patch -p1 < ../../tests/diag/trace-names.patch     # registra cada name_get (BF_TRACE=arquivo)
patch -p1 < ../../tests/diag/trace-draws.patch     # contador de sorteios e ponto de chamada (BF_WIN=ini-fim)
patch -p1 < ../../tests/diag/trace-units.patch     # sorteio e limiares de cada lance
cd ../..
BF_SRC=build/trace-src BF_BUILD=build/trace sh tools/build-upstream.sh
BF_TRACE=/tmp/trace.txt BYGFOOT_BIN=$PWD/build/trace/bygfoot sh spike/headless/bf-headless --seed 1
```

Ferramentas: `tools/first-divergence.py`, `tools/dump-state.sh`, `tools/state-digest.py`, `tools/state-sample.py`. Escrevem em arquivo apontado por `BF_TRACE` e são inócuas sem essa variável, mas alteram o binário: nunca distribuir.
