# Fase 0 — Spike e Diagnóstico do Bygfoot: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Descobrir, com evidência reproduzível, se o núcleo do Bygfoot roda uma partida sem GTK (gate G1) e decidir entre Caminho A (fork) e Caminho B (motor Rust).

**Architecture:** O upstream entra em `upstream/` sem edição. Todo diagnóstico é leitura e medição, feito por scripts em `tools/`. O spike headless fica em `spike/headless/`, compila arquivos do upstream sem alterá-los e substitui os símbolos de interface por stubs próprios. O resultado vira documentos em `docs/` e o `ADR-001`.

**Tech Stack:** C (clang/gcc), CMake ≥ 3.14, GLib, SQLite, shell POSIX, git. Rust e Tauri **não** entram na Fase 0.

**Spec:** `ROADMAP_BYGFOOT_MODERNO.md` e `CLAUDE.md` (nesta pasta). O roadmap visual com as decisões desta conversa está em https://claude.ai/artifact/NM8mABzasCPFKNLsr4brNU e prevalece onde o `.md` divergir (Task 1 sincroniza).

## Global Constraints

- Upstream: `https://gitlab.com/bygfoot/bygfoot.git`. Tag `3.0.0` = commit `767acfa`. O `HEAD` do clone de estudo (`9b7b006`) está **à frente** da tag (inclui o port GTK 3 e CMake). A Task 2 decide qual pinar.
- "Não alterar o upstream antes do diagnóstico" (CLAUDE.md). Nenhum arquivo de `upstream/` é editado nesta fase.
- Nenhuma afirmação de "executado/validado" sem comando e log salvos em `docs/spikes/`.
- Licença do upstream: GPL-2.0-only (conferir `upstream/COPYING` e os cabeçalhos). Não misturar código GPL-3.0 sem revisão.
- Nenhuma rotina de rede, analytics ou upload de saves no código novo.
- Dados e nomes fictícios no que for criado aqui.
- Instalar qualquer pacote na máquina (brew, cmake, etc.) só depois de a pessoa autorizar na hora. Os pacotes são listados antes.
- Plataformas-alvo do produto: macOS, Windows e Linux. A Fase 0 mede em macOS e deixa o workflow de CI escrito para os outros dois (não é publicado sem autorização).
- Datas em `YYYY-MM-DD`, textos de documentação em português do Brasil.

## Review Focus

- **Caminho com espaços e acentos** (`/Users/…/Meus Jogos/`): o spike e os scripts devem funcionar. Teste na Task 5.
- **Diretório de execução diferente:** o motor procura arquivos de definição (times, ligas) por caminho relativo. Rodar de outra pasta não pode dar resultado diferente nem travar. Teste na Task 5.
- **Seed 0 e seeds enormes:** `seed=0` e `seed=4294967295` precisam ser aceitas e repetíveis. Teste na Task 5.
- **Duas execuções simultâneas:** estado global no C não pode corromper uma execução paralela (processos separados, mesma seed, saída idêntica). Teste na Task 5.
- **Locale do sistema (pt_BR com vírgula decimal):** a saída do spike não pode mudar com `LC_ALL`. Teste na Task 5.

---

### Task 1: Sincronizar documentos com as decisões da conversa

**Files:**
- Modify: `CLAUDE.md`
- Modify: `ROADMAP_BYGFOOT_MODERNO.md`
- Create: `README.md`, `.gitignore`

**Interfaces:**
- Produces: `CLAUDE.md` com Caminho A como principal, macOS no MVP, regra de remendos mínimos no C (`patches/MANIFEST.md`), ordem de fases nova (partida ao vivo antes da gestão), e a camada de coreografia descrita como visual.

- [ ] **Step 1: Escrever o teste de documentação** `tools/check-docs.sh` que falha se faltar qualquer um destes textos: `macOS` na tabela de distribuição do MVP, `patches/MANIFEST.md` no `CLAUDE.md`, `coreografia` no roadmap, `Caminho A` marcado como principal.
- [ ] **Step 2: Rodar** `sh tools/check-docs.sh` — esperado: FAIL listando os 4 itens.
- [ ] **Step 3: Editar** `CLAUDE.md` e o roadmap (seções 1, 2.1, 4, 8 e 14) com as decisões. Em `.gitignore` incluir `build/`, `node_modules/`, `target/`, `*.log` fora de `docs/spikes/`.
- [ ] **Step 4: Rodar** `sh tools/check-docs.sh` — esperado: `OK`.
- [ ] **Step 5: Commit** `docs: sincroniza roadmap e CLAUDE.md com decisões do fork`.

### Task 2: Trazer o upstream sem editar e fixar a versão

**Files:**
- Create: `upstream/` (via git subtree), `docs/spikes/upstream-pin.md`
- Create: `tools/verify-upstream-pristine.sh`

**Interfaces:**
- Produces: `tools/verify-upstream-pristine.sh` sai com 0 quando `upstream/` é idêntico ao commit fixado. As tasks 3 a 7 e a Fase 1 chamam esse script.

- [ ] **Step 1: Escrever** `tools/verify-upstream-pristine.sh <commit>`: compara `git ls-tree`/hash de `upstream/` com o commit do remote `upstream`. Falha com lista de arquivos diferentes.
- [ ] **Step 2: Rodar** o script antes de existir `upstream/` — esperado: FAIL "upstream/ ausente".
- [ ] **Step 3: Adicionar** o remote `upstream` e rodar `git subtree add --prefix=upstream upstream <commit> --squash`. Pinar o commit escolhido e registrar em `docs/spikes/upstream-pin.md`: tag 3.0.0 (`767acfa`), `HEAD` estudado (`9b7b006`), diferença entre eles (`git log 3.0.0..9b7b006 --oneline`) e a escolha com motivo (recomendação: o commit com o port GTK 3, se o port não quebrar o núcleo).
- [ ] **Step 4: Rodar** o script — esperado: exit 0.
- [ ] **Step 5: Commit** `chore: adiciona upstream do Bygfoot em <commit>`.

### Task 3: Compilar o original em ambiente isolado (macOS)

**Files:**
- Create: `tools/build-upstream.sh`, `docs/spikes/build-macos.md`, `docs/spikes/build-macos.log`

**Interfaces:**
- Consumes: `upstream/` da Task 2.
- Produces: pasta `build/upstream/` (fora do git) com o binário original ou um relatório reproduzível do que falhou.

- [ ] **Step 1: Listar** as dependências exigidas por `upstream/CMakeLists.txt` e `README`/`INSTALL` (GTK 3, GLib, SQLite, gettext, cmake ≥ 3.14) e **pedir autorização** antes de instalar qualquer uma por Homebrew. Mostrar o comando exato.
- [ ] **Step 2: Escrever** `tools/build-upstream.sh`: `cmake -S upstream -B build/upstream && cmake --build build/upstream`, com toda a saída em `docs/spikes/build-macos.log`.
- [ ] **Step 3: Rodar** o script — esperado: binário gerado, ou falha registrada com a primeira linha de erro.
- [ ] **Step 4: Rodar** `sh tools/verify-upstream-pristine.sh <commit>` — esperado: exit 0 (o build não pode ter sujado `upstream/`).
- [ ] **Step 5: Documentar** em `docs/spikes/build-macos.md`: versões de ferramentas (`cmake --version`, `clang --version`), comando, resultado, tempo. Commit `docs(spike): build do upstream em macOS`.

### Task 4: Mapear o acoplamento com GTK e o estado global

**Files:**
- Create: `tools/gtk-coupling.sh`, `docs/REVERSE_ENGINEERING.md` (seções 1–3)

**Interfaces:**
- Produces: `tools/gtk-coupling.sh upstream/src` imprime uma tabela `arquivo | refs gtk | includes de gui | params GUI*gui | variáveis globais (extern)` em ordem alfabética, estável entre execuções. A Task 7 usa a lista de símbolos a serem stubados.

- [ ] **Step 1: Escrever o teste** `tools/test-gtk-coupling.sh`: roda o script duas vezes e exige saída idêntica; exige que `live_game.c` apareça com `params GUI*gui = 13` (valor medido na reconhecimento) e `fixture.c` com 0 includes de gui.
- [ ] **Step 2: Rodar** o teste — esperado: FAIL, script inexistente.
- [ ] **Step 3: Implementar** `tools/gtk-coupling.sh` com `grep`/`awk` sobre `upstream/src/*.c *.h`.
- [ ] **Step 4: Rodar** o teste — esperado: PASS. Se a contagem de `live_game.c` diferir de 13, corrigir o script ou registrar a diferença com motivo.
- [ ] **Step 5: Escrever** em `docs/REVERSE_ENGINEERING.md`: (1) mapa de `src/` por responsabilidade, (2) tabela do script, (3) cadeia de chamadas de `live_game_calculate_fixture` e `live_game_create_unit` até as funções que tocam GUI, com arquivo e linha. Commit `docs(re): acoplamento GTK e estado global`.

### Task 5: Spike headless — uma partida sem janela

**Files:**
- Create: `spike/headless/main.c`, `spike/headless/gui_stubs.c`, `spike/headless/CMakeLists.txt`
- Create: `tests/spike/test-headless.sh`
- Create: `docs/spikes/headless-result.md`

**Interfaces:**
- Consumes: `upstream/src/*.c` (somente leitura), lista de símbolos da Task 4.
- Produces: executável `bf-headless` com `bf-headless --seed <u32> --home <id> --away <id>` que escreve uma linha JSON por `LiveGameUnit` no stdout: `{"minute":int,"time":int,"possession":0|1,"area":0|1|2,"event":int,"team":int,"result":[int,int]}` e termina com a linha `{"final":[int,int]}`. Código de saída 0 em sucesso, 2 se o motor não pôde ser inicializado sem GTK.

- [ ] **Step 1: Escrever os testes** em `tests/spike/test-headless.sh`, todos comparando saída byte a byte:
  - mesma seed duas vezes → idêntico;
  - `--seed 0` e `--seed 4294967295` → saem com 0 e repetem;
  - rodar de `/tmp/dir com espaço é` e de `/` → saída igual à da pasta do projeto;
  - `LC_ALL=pt_BR.UTF-8` e `LC_ALL=C` → saída igual;
  - duas execuções simultâneas com a mesma seed (`&` e `wait`) → as duas idênticas ao baseline;
  - gols nos eventos (`event` de gol) somam o `final`.
- [ ] **Step 2: Rodar** `sh tests/spike/test-headless.sh` — esperado: FAIL (sem executável).
- [ ] **Step 3: Implementar** `spike/headless/CMakeLists.txt` compilando só os `.c` do upstream que o núcleo precisa, com `-DBYGFOOT_HEADLESS`, sem `find_package(GTK3)`. Implementar `gui_stubs.c` com **as funções de interface que o linker pedir**, cada uma vazia e registrada em `docs/spikes/headless-result.md` (nome, motivo). Implementar `main.c` conforme a interface acima. Regra: se for preciso alterar um arquivo de `upstream/`, **não alterar**: registrar o ponto exato (arquivo, linha, motivo) como candidato a remendo.
- [ ] **Step 4: Rodar** o teste — esperado: PASS completo, ou falhas restantes documentadas com a causa. Rodar também `tools/verify-upstream-pristine.sh`.
- [ ] **Step 5: Documentar** o resultado: lista de stubs, candidatos a remendo, o que não foi possível, saída de exemplo. Commit `feat(spike): bf-headless simula uma partida sem GTK`.

### Task 6: Mapear o save SQLite e as regras de campeonato

**Files:**
- Create: `tools/sqlite-schema.sh`, `docs/REVERSE_ENGINEERING.md` (seções 4–5)

**Interfaces:**
- Produces: `tools/sqlite-schema.sh upstream/src` extrai todos os `CREATE TABLE`/`CREATE INDEX` do código para stdout, ordenados, com arquivo e linha de origem. A Fase 2 usa essa lista para o `SimulationAdapter` e o importador.

- [ ] **Step 1: Escrever o teste** que roda o script duas vezes (saída igual) e exige que ao menos uma tabela de jogadores e uma de partidas apareçam. Esperado: FAIL.
- [ ] **Step 2: Implementar** o script sobre `grep -rn`.
- [ ] **Step 3: Rodar** o teste — esperado: PASS.
- [ ] **Step 4: Escrever** as seções 4–5: tabelas e colunas do save, e como `league.c`/`fixture.c` geram turno e returno, desempate e calendário (com função e linha).
- [ ] **Step 5: Commit** `docs(re): schema SQLite e regras de campeonato`.

### Task 7: Revisão de licenças e assets

**Files:**
- Create: `tools/license-scan.sh`, `docs/LEGAL_REVIEW.md`

**Interfaces:**
- Produces: `tools/license-scan.sh upstream` lista, por arquivo `.c/.h`, o identificador de licença do cabeçalho, e, por pasta de assets (`support_files/`), os tipos de arquivo e se existe nota de licença.

- [ ] **Step 1: Escrever o teste** exigindo que a saída tenha uma linha por arquivo fonte e que conte quantos arquivos não têm cabeçalho reconhecido. Esperado: FAIL.
- [ ] **Step 2: Implementar** o script.
- [ ] **Step 3: Rodar** o teste — esperado: PASS.
- [ ] **Step 4: Escrever** `docs/LEGAL_REVIEW.md`: licença do upstream (conferir `COPYING`), arquivos sem cabeçalho, assets e **nomes reais de clubes/jogadores** em `support_files/`, licenças das dependências planejadas (GLib LGPL, SQLite domínio público, Tauri com escolha MIT, nota de que Apache-2.0 não combina com GPL-2.0-only) e as perguntas que exigem uma pessoa com conhecimento jurídico. **Não concluir** que algo é permitido: só listar fatos e dúvidas.
- [ ] **Step 5: Commit** `docs(legal): revisão inicial de licenças e assets`.

### Task 8: Decisão G1 e ADR-001

**Files:**
- Create: `docs/adr/ADR-001-origem-do-codigo.md`, `.github/workflows/spike.yml`
- Modify: `docs/REVERSE_ENGINEERING.md` (seção 6: plano do spike e resultado)

**Interfaces:**
- Consumes: resultados das Tasks 3 a 7.
- Produces: decisão registrada: **seguir** (Caminho A), **corrigir** (processo separado com JSON) ou **pivotar** (Caminho B, motor Rust), com os links para os logs.

- [ ] **Step 1: Escrever** `.github/workflows/spike.yml` com matriz `macos-latest`, `windows-latest`, `ubuntu-latest` que compila `spike/headless` e roda `tests/spike/test-headless.sh`. **Não publicar:** o arquivo fica só local até a pessoa autorizar o repositório remoto.
- [ ] **Step 2: Preencher** o `ADR-001` com o formato Contexto / Decisão / Consequências, citando evidências (logs e testes da Task 3 e 5) e o número de candidatos a remendo da Task 5.
- [ ] **Step 3: Verificar** o gate G1 e escrever o resultado literal: o `bf-headless` simula uma partida sem GTK, com resultado repetível em macOS? Sim ou não, com o comando que prova.
- [ ] **Step 4: Rodar** `sh tools/check-docs.sh && sh tools/verify-upstream-pristine.sh <commit> && sh tests/spike/test-headless.sh` — esperado: tudo exit 0, ou falhas explicadas no ADR.
- [ ] **Step 5: Commit** `docs(adr): ADR-001 e resultado do gate G1`. Apresentar à pessoa a decisão e os próximos passos da Fase 1 ou do pivô.

---

## Self-review

- **Cobertura do spec:** `docs/REVERSE_ENGINEERING.md`, `docs/LEGAL_REVIEW.md`, plano e resultado do spike, ADR-001 e gate G1 (Sprint 01 e 02 do roadmap, BF-001 a BF-005, BF-010) estão nas Tasks 1 a 8. O GDD e os wireframes (BF-006) ficam **fora** deste plano: não bloqueiam o G1 e entram no começo da Fase 2.
- **Tipos:** a saída JSON do `bf-headless` na Task 5 usa os mesmos campos de `LiveGameUnit` lidos em `live_game_struct.h` (`possession`, `area`, `minute`, `time`, `result`, `event`).
- **Proporção:** o plano tem 8 tasks e quase nenhum código, porque o trabalho é investigação. As decisões que o executor não poderia tomar sozinho (formato de saída, pino do upstream, regra de não editar, testes de borda) estão escritas.
