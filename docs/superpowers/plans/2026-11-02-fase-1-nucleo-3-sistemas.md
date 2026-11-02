# Fase 1 — Núcleo headless nos 3 sistemas: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fechar o que o G1 deixou em aberto: provar (ou refutar) que a mesma seed dá a mesma partida em macOS, Linux e Windows, dar versão ao contrato de saída, e montar a infraestrutura que mantém o fork atualizável.

**Architecture:** O `upstream/` continua intacto. Os resultados são medidos pelo wrapper `spike/headless/bf-headless` (que será promovido a `core/` só se a Fase 1 mostrar que vale a pena). Um CI em três sistemas calcula hashes de partidas com seeds fixas e um job final compara os hashes entre sistemas. Remendos futuros viverão em `patches/*.patch` e serão aplicados a uma cópia, nunca ao `upstream/`.

**Tech Stack:** shell POSIX, Python 3 (só biblioteca padrão), git (subtree), GitHub Actions (macOS, Ubuntu, Windows/MSYS2), `patch`. Rust e Tauri **não** entram: começam na Fase 2.

**Spec:** `ROADMAP_BYGFOOT_MODERNO.md` (Sprint 02, gate G1), `CLAUDE.md`, `docs/adr/ADR-001-origem-do-codigo.md` (as condições abertas) e `docs/spikes/headless-result.md`. O roadmap visual (https://claude.ai/artifact/NM8mABzasCPFKNLsr4brNU) prevalece onde divergir.

## Global Constraints

- `upstream/` nunca é editado (`sh tools/verify-upstream-pristine.sh 9b7b0062` deve sair 0 ao fim de cada task). Remendos vão em `patches/`.
- Nenhuma afirmação de "funciona" sem comando e saída (para CI: o número da execução).
- Seeds válidas: `0..2147483647` (ver ADR-001). `bf-headless` fixa `LC_ALL=C`.
- Sem rede, analytics ou envio de saves em código do produto. (Workflows de CI podem usar a rede.)
- Dados e nomes de clubes reais só como entrada de teste local; nada disso vai para o produto (`docs/LEGAL_REVIEW.md`).
- Alvos: macOS, Linux e Windows (MSYS2/MinGW64 no CI). Hashes e comparações usam bytes com `\n` e UTF-8.
- O repositório remoto é público (`andre-zaguette/bygfoot-moderno`). **Este plano só envia a branch `fase-1-nucleo` ao `origin`, para o CI rodar. O merge em `main` precisa de confirmação da pessoa responsável.**
- Sem instalar nada novo na máquina local; no CI, só pacotes via `apt`, `brew` e `pacman` (MSYS2) já usados mais `patch` e `python` quando faltarem.

## Review Focus

- **Time inexistente (`--team "Nome que não existe"`):** o motor cai (hoje: exit 11). O wrapper deve sair com 3, sem nada no stdout, com mensagem clara. Teste na Task 1.
- **`--weeks` maior que a temporada (ex.: 300):** o motor cai. O wrapper deve recusar o valor com 2 acima de um teto documentado, em vez de deixar o motor cair. Teste na Task 1.
- **Nomes com acento em todos os sistemas (`B. Tórshavn`):** a seed de ouro com acento deve gerar o mesmo hash nos três. Task 2.
- **Fim de linha e codificação no hash:** o hash é de bytes UTF-8 com `\n`; uma diferença de CRLF não pode passar por "igual". Task 2.
- **Remendo que não encaixa mais depois de um sync (arquivo renomeado, contexto mudou):** `apply-patches` deve falhar sem deixar cópia parcial. Task 5.

---

### Task 1: Seeds de ouro e entradas de borda do wrapper

**Files:**
- Create: `tests/golden-seeds/seeds.txt`, `tests/golden-seeds/expected-macos.txt`, `tools/golden-hash.py`, `tests/golden-seeds/test-golden.sh`
- Modify: `spike/headless/bf-headless`
- Test: `tests/spike/test-headless-args.sh`

**Interfaces:**
- Produces: `python3 -I tools/golden-hash.py [--seeds ARQUIVO]` imprime uma linha por seed: `<seed>\t<team>\t<weeks>\t<sha256-hex>\t<linhas>`, em UTF-8 com `\n`, ordem do arquivo. O sha256 é do stdout completo de `bf-headless` (bytes). Código de saída 1 se alguma execução falhar.
- Produces: formato de `seeds.txt`: uma linha `seed|team|weeks`; `#` inicia comentário. Conteúdo mínimo: seeds `0`, `1`, `431199`, `2147483647`, `20261102`, e `431199|B. Tórshavn|2`.

- [ ] **Step 1: Escrever os testes de borda** em `tests/spike/test-headless-args.sh`: `--team "Time Inexistente"` sai com 3, stdout vazio, stderr contém `time`; `--weeks 300` sai com 2 e stderr cita o teto. Rodar e ver o primeiro passar (já sai 3) e o segundo falhar (hoje sai 3 após queda do motor).
- [ ] **Step 2: Medir o teto de `--weeks`** rodando seeds de borda com `--weeks 20..60` e registrando o maior valor que não derruba o motor em `docs/spikes/weeks-limit.md` (comando e saída). Usar esse valor menos uma margem como `MAX_WEEKS` no wrapper.
- [ ] **Step 3: Implementar** a validação `weeks <= MAX_WEEKS` (exit 2 com mensagem) e a mensagem de time inexistente (exit 3, texto `time não encontrado ou motor caiu`) em `bf-headless`.
- [ ] **Step 4: Escrever o teste de ouro** `tests/golden-seeds/test-golden.sh`: roda `golden-hash.py` e compara byte a byte com `expected-macos.txt`; falha com a primeira seed que diverge. Rodar: FAIL (falta `expected-macos.txt` e `golden-hash.py`).
- [ ] **Step 5: Implementar `tools/golden-hash.py`**, gerar `expected-macos.txt` no macOS, rodar o teste: PASS.
- [ ] **Step 6: Commit** `feat(core): seeds de ouro e validação de entradas do wrapper`.

### Task 2: Comparação entre sistemas no CI

**Files:**
- Create: `tools/compare-golden.py`, `tests/golden-seeds/test-compare-golden.sh`
- Modify: `.github/workflows/spike.yml`

**Interfaces:**
- Consumes: `tools/golden-hash.py` (Task 1).
- Produces: `python3 -I tools/compare-golden.py ARQ1 ARQ2 [ARQ3…]` sai 0 se todos os arquivos têm as mesmas linhas; sai 1 imprimindo, para cada seed divergente, `seed team weeks` e os hashes por arquivo; sai 2 se um arquivo faltar.

- [ ] **Step 1: Escrever** `test-compare-golden.sh` com três casos sintéticos: arquivos iguais → 0; um hash diferente → 1 com a seed na saída; arquivo ausente → 2. Rodar: FAIL.
- [ ] **Step 2: Implementar** `compare-golden.py`. Rodar: PASS.
- [ ] **Step 3: Estender o workflow:** cada job da matriz roda `golden-hash.py` e publica `golden-<os>.txt` como artefato; um job final `compare` baixa os três e roda `compare-golden.py`. Em MSYS2, instalar `patch`.
- [ ] **Step 4: Enviar a branch `fase-1-nucleo` ao `origin` e acompanhar o CI.** Registrar o número da execução e o resultado do job `compare` em `docs/spikes/cross-os-determinism.md`. **Expected:** ou `compare` verde (os 3 sistemas dão o mesmo resultado) ou vermelho com as seeds divergentes listadas. Os dois resultados são válidos; o documento diz qual aconteceu.
- [ ] **Step 5: Commit** `ci: compara partidas de seeds de ouro entre macOS, Linux e Windows`.

### Task 3: Diagnóstico de divergência (só se a Task 2 divergir)

**Files:**
- Create: `tools/first-divergence.py`, `tests/golden-seeds/test-first-divergence.sh`, `docs/adr/ADR-002-determinismo-entre-sistemas.md`

**Interfaces:**
- Produces: `python3 -I tools/first-divergence.py SAIDA_A SAIDA_B` imprime o índice e o conteúdo do primeiro lance diferente entre duas saídas de `bf-headless`; sai 0 se iguais, 1 se diferentes.

- [ ] **Step 1: Se a Task 2 deu verde, pular esta task** e escrever no ADR-002 a evidência da igualdade (execução do CI) e que nada precisa de remendo.
- [ ] **Step 2: Escrever o teste** de `first-divergence.py` com dois arquivos sintéticos que divergem no lance 3. Rodar: FAIL. Implementar. PASS.
- [ ] **Step 3: Baixar as saídas completas das seeds divergentes** (o job passa a guardá-las) e achar o primeiro lance diferente. Classificar a causa: ponto flutuante, ordem de ordenação/hash, GLib `GRand` entre versões, localidade, ou outra, com a evidência.
- [ ] **Step 4: Decidir no ADR-002:** contornar no wrapper, remendo no `patches/` (Task 5) ou aceitar que os resultados não são comparáveis entre sistemas (com consequência para save/replay compartilhado). Commit `docs(adr): ADR-002 determinismo entre sistemas`.

### Task 4: Contrato versionado `MatchTimeline v1`

**Files:**
- Create: `schemas/match-timeline-v1.schema.json`, `tests/contract/test_timeline_v1.py`, `tests/contract/timeline-v1.sample.json`
- Modify: `spike/headless/bf-headless`, `spike/headless/extract_units.py`

**Interfaces:**
- Produces: `bf-headless --seed N … --format timeline-v1` imprime **um** documento JSON: `{"schemaVersion":1,"source":{"engine":"bygfoot","upstreamCommit":"9b7b0062…","seed":N,"country":…,"team":…,"weeks":…},"match":{"fixtureId":int,"home":str,"away":str,"final":[int,int]},"units":[{"minute":int,"time":0..3,"possession":0|1,"area":0|1|2,"result":[int,int],"event":{"type":int,"team":int,"player":{"id":int,"name":str}?,"player2":{…}?,"commentary":str?}}]}`. Sem `--format`, a saída continua em linhas, como hoje.
- Invariantes testadas: `schemaVersion == 1`; `units` não vazio; `area` em 0..2; `possession` em 0..1; `time` em 0..3; último `result` igual a `match.final`; mudança de placar só em lances de gol (tipos 10 e 11).

- [ ] **Step 1: Escrever** `test_timeline_v1.py` (só biblioteca padrão) com as invariantes acima e comparação byte a byte da saída da seed `431199` com `timeline-v1.sample.json`. Rodar: FAIL.
- [ ] **Step 2: Escrever** o schema JSON (documentação do contrato; o teste valida as invariantes em código, não via biblioteca de schema).
- [ ] **Step 3: Implementar** `--format timeline-v1` em `bf-headless`/`extract_units.py`, aproveitando `player`, `player2` e `commentary` que o dump já traz. Gerar o `sample.json`. Rodar: PASS.
- [ ] **Step 4: Teste de rejeição:** `--format xyz` sai com 2. Commit `feat(contract): MatchTimeline v1 versionado com teste de contrato`.

### Task 5: Infraestrutura de remendos (`patches/`)

**Files:**
- Create: `patches/MANIFEST.md`, `tools/apply-patches.sh`, `tests/patches/test-apply-patches.sh`

**Interfaces:**
- Produces: `sh tools/apply-patches.sh DESTINO` copia `upstream/` para `DESTINO` (que não pode existir) e aplica `patches/*.patch` em ordem. Variáveis `UPSTREAM_DIR` e `PATCH_DIR` substituem os padrões, para teste. Sai 0 se tudo aplicou; sai 1 se um remendo não encaixa, se há `.patch` fora do `MANIFEST.md` ou se o `MANIFEST.md` lista um arquivo que não existe; nos casos de erro remove `DESTINO` (nada parcial).
- `MANIFEST.md`: tabela `arquivo.patch | arquivo do upstream | motivo | candidato nº (docs/spikes/headless-result.md)`. Começa sem remendos.

- [ ] **Step 1: Escrever os testes** com um upstream e remendos sintéticos em pasta temporária: aplica e confere o conteúdo; remendo com contexto errado → exit 1 e `DESTINO` removido; `.patch` fora do manifesto → exit 1; manifesto com arquivo inexistente → exit 1; `DESTINO` já existente → exit 2. Rodar: FAIL.
- [ ] **Step 2: Implementar** `apply-patches.sh` com `patch -p1 --dry-run` de todos antes de aplicar de verdade. PASS.
- [ ] **Step 3: Rodar** o script sobre o `upstream/` real sem remendos: `DESTINO` idêntico ao `upstream/` (`diff -r`). Commit `feat(patches): infraestrutura de remendos aplicados a uma cópia`.

### Task 6: Sincronização com o upstream

**Files:**
- Create: `tools/sync-upstream.sh`, `tests/patches/test-sync-upstream.sh`, `.github/workflows/sync-upstream.yml`, `docs/spikes/sync-experiment.md`

**Interfaces:**
- Consumes: `tools/apply-patches.sh` (Task 5), `tools/golden-hash.py` (Task 1), `tools/upstream-pin.txt`.
- Produces: `sh tools/sync-upstream.sh COMMIT_COMPLETO` num clone limpo: faz `git subtree pull` do commit, atualiza `tools/upstream-pin.txt`, aplica remendos numa cópia, compila, roda as seeds de ouro e imprime `SEEDS_IGUAIS` ou `SEEDS_MUDARAM <lista>`; sai 0 em ambos os casos (a decisão de aceitar é humana) e 1 se o merge, os remendos ou o build falharem.

- [ ] **Step 1: Escrever o teste** (lento, marcado como tal): num clone temporário, fixar o upstream na tag `3.0.0` (`767acfa…`), rodar `sync-upstream.sh 9b7b0062ddb90a620865314dddf0c4b1c0a23660` e conferir que imprime `SEEDS_IGUAIS` ou `SEEDS_MUDARAM`, que `upstream-pin.txt` passou a apontar o novo commit e que `verify-upstream-pristine.sh` passa. Rodar: FAIL (script ausente).
- [ ] **Step 2: Implementar** `sync-upstream.sh`. PASS.
- [ ] **Step 3: Registrar o experimento** em `docs/spikes/sync-experiment.md`: as seeds de ouro mudam entre `3.0.0` e `9b7b0062` (10 commits de diferença)? Essa é a medida real do risco de acompanhar o upstream.
- [ ] **Step 4: Workflow `sync-upstream.yml`** (`workflow_dispatch` e semanal): busca o `main` do upstream, roda o script e abre um PR de rascunho `sync/upstream-<sha>` com o resultado; permissões mínimas `contents: write` e `pull-requests: write`. Verificar uma vez com `workflow_dispatch` na branch (sem commits novos: deve terminar sem PR). Commit.

### Task 7: Fechamento da fase

**Files:**
- Modify: `docs/adr/ADR-001-origem-do-codigo.md`, `ROADMAP_BYGFOOT_MODERNO.md`, `README.md`, `.github/workflows/spike.yml` (renomear para `ci.yml` se fizer sentido)

- [ ] **Step 1: Atualizar o ADR-001:** as condições "dados versionados" (Task 4) e "igualdade entre sistemas" (Tasks 2 e 3) passam a ter o estado real, com links para as execuções do CI.
- [ ] **Step 2: Rodar a suíte completa** local: `tools/check-docs.sh`, `verify-upstream-pristine.sh`, todos os `test-*.sh` e `tests/**`. Esperado: tudo exit 0.
- [ ] **Step 3: Conferir o CI** da branch verde nos três sistemas, incluindo `compare` (ou o ADR-002 explicando a divergência).
- [ ] **Step 4: Pedir confirmação para o merge** em `main`; só então abrir o PR ou fazer o merge. Commit `docs: fecha a Fase 1`.

---

## Self-review

- **Cobertura do spec:** Sprint 02 do roadmap (contrato versionado `SimulationAdapter`, seeds fixas, atualizações do upstream) e as 5 condições do ADR-001 estão nas Tasks 1–7. **Fora do plano, de propósito:** o app Tauri, a ponte Rust e o `SimulationAdapter` em Rust (Fase 2), e a extração de biblioteca sem GTK (só se a Fase 1 mostrar necessidade; o ADR-001 deixa em aberto).
- **Tipos:** o formato de linha de `golden-hash.py` (Task 1) é o mesmo lido por `compare-golden.py` (Task 2); `upstream-pin.txt` (já existente) é lido e escrito pela Task 6; o `MatchTimeline v1` (Task 4) usa os campos de `LiveGameUnit` já extraídos pelo spike.
- **Proporção:** 7 tasks, sem código além de assinaturas, formatos e invariantes.
- **Risco principal:** a Task 3 é condicional. Se a Task 2 divergir, o custo da fase muda e o ADR-002 pode concluir que os resultados não são comparáveis entre sistemas, o que afeta saves e replays compartilhados.
