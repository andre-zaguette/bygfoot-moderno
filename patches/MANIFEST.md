# Remendos sobre o upstream

Cada remendo é aplicado a uma **cópia** de `upstream/` por `tools/apply-patches.sh`. `upstream/` nunca é editado.
Todo `.patch` desta pasta precisa de uma linha abaixo; o script falha se faltar.

| arquivo.patch | arquivo do upstream | motivo | candidato nº (docs/spikes/headless-result.md) |
|---|---|---|---|
| 0001-ordenar-listagem-de-diretorios.patch | src/file.c | ADR-002: ordenar `g_dir_read_name` para a ordem de carga não depender do sistema de arquivos | 6 (determinismo) |
| 0002-ordem-de-avaliacao-nome-sobrenome.patch | src/name.c | ADR-002: `sprintf` com duas chamadas de sorteio nos argumentos; fixa nome antes do sobrenome | 6 (determinismo) |
| 0003-ordem-de-avaliacao-transferencias.patch | src/transfer.c | ADR-002: `transfer_add_player(transfer_player_get_new(math_rnd..), math_rndi..)` tem dois argumentos que sorteiam; fixa a ordem esquerda→direita | 6 (determinismo) |
