# Remendos sobre o upstream

Cada remendo é aplicado a uma **cópia** de `upstream/` por `tools/apply-patches.sh`. `upstream/` nunca é editado.
Todo `.patch` desta pasta precisa de uma linha abaixo; o script falha se faltar.

| arquivo.patch | arquivo do upstream | motivo | candidato nº (docs/spikes/headless-result.md) |
|---|---|---|---|
| 0001-ordenar-listagem-de-diretorios.patch | src/file.c | EXPERIMENTO (ADR-002): ordenar `g_dir_read_name` para a ordem de carga não depender do sistema de arquivos | 6 (hipótese de determinismo) |
| 0002-ordem-de-avaliacao-nome-sobrenome.patch | src/name.c | EXPERIMENTO (ADR-002): `sprintf` com duas chamadas de sorteio nos argumentos; fixa nome antes do sobrenome | 6 (hipótese de determinismo) |
