# Experimento: as seeds mudam entre versões do upstream? (Fase 1, Task 6)

## Pergunta

O fork precisa continuar atualizável. O risco real de acompanhar o upstream é uma atualização mudar o resultado de uma seed fixa (por refatorar a ordem dos sorteios, por exemplo), o que invalidaria saves e replays.

## Medida

Workflow `.github/workflows/upstream-drift.yml`, execução **41480817577** (Ubuntu, mesmo compilador, **sem remendos**): compila a tag `3.0.0` (`767acfa`, GTK2) e o commit fixado `9b7b0062` (GTK3), e roda as 6 seeds de ouro nas duas.

Os 10 commits entre elas (lista no artefato `drift-commits.txt`) incluem o port GTK2→GTK3, refatorações de `fixture`/`cup`/`league` e "Optimize player_update_week_roundly".

## Resultado

**Os 6 hashes são idênticos** entre `3.0.0` e `9b7b0062`, e iguais aos hashes do Ubuntu original da execução de determinismo (`6616d682…` na seed 0). Nenhuma das 10 mudanças alterou essas partidas.

## O que isto não prova

- É uma amostra: 6 seeds, 2 semanas, um país, um sistema. Não cobre fim de temporada, copas, transferências em semanas avançadas.
- Mede deriva entre duas versões já existentes. Qualquer sincronização futura deve rodar `tools/sync-upstream.sh`, que imprime `SEEDS_IGUAIS` ou `SEEDS_MUDARAM <seeds>`, e as baterias do CI (6 seeds e 16 seeds por 30 semanas).
- O uso de `patches/` continua sendo o ponto frágil: um remendo pode deixar de encaixar depois de um sync. `apply-patches.sh` falha sem deixar cópia parcial (testado) e o `sync-upstream.sh` repassa a falha.

## Teste do script

`tests/patches/test-sync-upstream.sh` (lento, precisa de rede): num clone temporário troca `upstream/` pela `3.0.0`, sincroniza com o commit fixado e confere `SEEDS_IGUAIS`, o pin atualizado e `verify-upstream-pristine.sh`. Resultado nesta sessão: `OK`. Recusa também argumento ausente, commit curto e árvore suja. O ramo `SEEDS_MUDARAM` foi checado só na lógica de comparação (`awk`) com arquivos sintéticos.
