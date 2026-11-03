# Fase 3: partida ao vivo em 2D (B + C) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (execução nativa escolhida pelo usuário). Steps usam checkbox (`- [ ]`).

**Goal:** assistir a partida simulada num campo 2D animado a 60 fps, com visual de transmissão (B) e uma camada de análise opcional (C), tocando a `MatchTimeline v1` do motor em ritmo de jogo.

**Architecture:** três camadas separadas. (1) `playback`: relógio puro que transforma a timeline em tempo real (pausa, 1x/2x/4x, pular). (2) `choreography`: função pura e determinística que converte cada lance (área, posse, tipo) em quadros-chave de bola e jogadores; nunca altera placar. (3) `PitchView`: renderer PixiJS 8 que só desenha o quadro atual. HUD e controles são React. Nada na interface decide resultado.

**Tech Stack:** PixiJS 8 (única dependência nova, versão exata fixada na Task 4), React, TypeScript, Vitest, Tauri 2 (já existentes).

**Spec:** `ROADMAP_BYGFOOT_MODERNO.md` (seções 6 e 7), `CLAUDE.md` (regras 2, 3, 4 e política de UI), `docs/adr/ADR-003-*.md`, visual escolhido pelo usuário: B (transmissão) + C (análise)

## Global Constraints

- O renderer e a coreografia **não calculam resultado**: placar, autor e minuto vêm só dos lances do motor (regra 2).
- O campo é uma **aproximação visual**: a interface mostra esse rótulo (regra 3). O motor não dá coordenadas.
- Mesma timeline = mesmos quadros (coreografia determinística, sem `Math.random` nem relógio; qualquer variação usa PRNG com seed derivada da timeline) (regra 4).
- Times, camisas e nomes fictícios. Sem escudos ou atletas reais (regra 7).
- Sem rede, sem analytics (CLAUDE.md). Só a dependência `pixi.js`, instalada dentro do repositório.
- Acessibilidade: foco visível, teclado completo, `prefers-reduced-motion` respeitado, contraste AA, `aria-live` para lances importantes. Validar em 1366×768 e 1920×1080, claro e escuro.
- Tokens em `design-system/MASTER.md` **antes** de codificar a tela (política de UI).
- Evidência: comando e resultado para cada afirmação; captura de tela para o visual.

## Review Focus

- Lance com `minute: -1` (sem minuto próprio): o relógio não pode voltar nem travar.
- Pênaltis (`time: 3`): placar de disputa não é gol de jogo; coreografia e relógio não podem inventar minutos.
- Timeline com 1 lance, sem `commentary`, ou só com START e END: tela não quebra.
- Cores de camisa dos dois times indistinguíveis ou sem contraste com a grama.
- Pausa, troca de velocidade e "pular para o fim" no meio de uma animação: o placar final exibido é sempre `match.final`.
- Janela redimensionada e aba em segundo plano (ticker do navegador pausa): o relógio não pode acumular salto enorme.
- Movimento reduzido: sem animação contínua; lances aparecem por troca de estado.

---

### Task 1: Tokens de design e kits fictícios

**Files:**
- Create: `design-system/MASTER.md`, `app/src/design/tokens.ts`, `app/src/design/tokens.test.ts`

**Interfaces:**
- Produces: `tokens.pitch` (cores da grama em faixas, linhas), `tokens.kits.home|away` (camisa, borda), `tokens.overlay` (nomes, rede, calor), `contrast(a: string, b: string): number` (razão WCAG).

- [ ] **Step 1:** Testes: `contrast(kit.home, kit.away) >= 3`; cada camisa contra as duas faixas da grama `>= 3`; texto do HUD contra o fundo `>= 4.5`; o mesmo nos temas claro e escuro.
- [ ] **Step 2:** Rodar `cd app && npm test`. Expected: FAIL (módulo não existe).
- [ ] **Step 3:** Implementar `tokens.ts` e documentar em `MASTER.md` (paleta B, camada C, tipografia, espaçamento, estados vazio/erro/carregando).
- [ ] **Step 4:** Rodar de novo. Expected: PASS.
- [ ] **Step 5:** Commit `feat(fase-3): tokens de design e kits ficticios`.

### Task 2: Relógio de reprodução (`playback`)

**Files:**
- Create: `app/src/live/playback.ts`, `app/src/live/playback.test.ts`

**Interfaces:**
- Produces: `createPlayback(units: Unit[], opts?: { secondsPerMinute?: number }): Playback` com `tick(dtMs: number): void`, `play()`, `pause()`, `setSpeed(1|2|4)`, `seekToEnd()`, `state(): { index: number; progress: number; minute: number; playing: boolean; speed: 1|2|4; finished: boolean }`. Padrão: 1x = 4 s reais por minuto de jogo (90 min ≈ 6 min); 4x ≈ 1,5 min.

- [ ] **Step 1:** Testes (cada um nomeia a mudança que o quebraria): avança com `tick`; velocidade escala o avanço; pausa congela; `minute: -1` usa o minuto do lance anterior e não regride; pênaltis (`time: 3`) avançam por tempo fixo; `dtMs` gigante (aba em segundo plano) é limitado a 250 ms; `seekToEnd` termina em `index = units.length - 1` e `finished`; timeline de 1 lance não quebra; o índice nunca diminui.
- [ ] **Step 2:** `npm test -- playback`. Expected: FAIL.
- [ ] **Step 3:** Implementar `playback.ts` (puro, sem DOM, sem `Date.now`).
- [ ] **Step 4:** `npm test -- playback`. Expected: PASS.
- [ ] **Step 5:** Commit.

### Task 3: Coreografia determinística (`choreography`)

**Files:**
- Create: `app/src/live/choreography.ts`, `app/src/live/choreography.test.ts`

**Interfaces:**
- Consumes: `Unit`, `Timeline` de `app/src/types.ts`.
- Produces: `buildChoreography(t: Timeline): Frame[]` (um `Frame` por lance) com `Frame = { ball: Vec; players: { home: Vec[11]; away: Vec[11] }; effect: Effect | null; score: [number, number]; caption: string }`, coordenadas em metros num campo de 105×68. `Effect` cobre gol, defesa, trave, escanteio, falta, cartão amarelo/vermelho, substituição, mudança de estilo. Formação fixa 4-4-2 deslocada pela área do lance e pela posse.

- [ ] **Step 1:** Testes: determinismo (duas chamadas, `toEqual`); `frame.score` de cada quadro é exatamente `unit.result` (a coreografia não calcula placar); todas as coordenadas dentro de 0..105 × 0..68; gol termina com a bola dentro da baliza do time certo; os 22 jogadores sempre existem; sem `commentary`, `caption` vira texto neutro; rodar sobre a amostra do contrato e sobre 3 timelines reais (script gera em `app/src/live/__fixtures__/` com o motor; comitar JSON pequeno).
- [ ] **Step 2:** `npm test -- choreography`. Expected: FAIL.
- [ ] **Step 3:** Implementar. Variações de posição usam um PRNG (mulberry32) com seed derivada de `source.seed` e do índice do lance.
- [ ] **Step 4:** `npm test -- choreography`. Expected: PASS.
- [ ] **Step 5:** Commit.

### Task 4: Renderer PixiJS (`PitchView`, visual B)

**Files:**
- Modify: `app/package.json` (adicionar `pixi.js` com versão exata, escolhida com `npm view pixi.js version` e pelo menos 2 semanas de idade)
- Create: `app/src/live/PitchView.tsx`, `app/src/live/interpolate.ts`, `app/src/live/interpolate.test.ts`

**Interfaces:**
- Consumes: `Frame`, `Playback.state()`.
- Produces: `<PitchView frames playback reducedMotion />`; `lerpFrame(a: Frame, b: Frame, t: number): Frame` (puro, testado).

- [ ] **Step 1:** Testes de `lerpFrame`: `t = 0` igual a `a`, `t = 1` igual a `b`, `t = 0.5` no meio, nunca fora do campo; com movimento reduzido, devolve sempre `b`.
- [ ] **Step 2:** `npm test -- interpolate`. Expected: FAIL.
- [ ] **Step 3:** Implementar `interpolate.ts` e `PitchView.tsx` (grama em faixas, linhas, camisas com sombra, bola; redimensiona com o contêiner; destrói a aplicação ao desmontar).
- [ ] **Step 4:** `npm test` e `npm run build`. Expected: PASS e build sem erro.
- [ ] **Step 5:** Verificação visual: `npm run dev`, abrir no navegador, capturar tela em 1366×768 e 1920×1080 (claro e escuro) e medir o tempo de quadro por 10 s. Registrar números em `docs/spikes/fase-3-desempenho.md`. Expected: mediana ≤ 16,7 ms numa máquina de desenvolvimento; a medição no WebView do Windows e do Linux é limite conhecido (CI não mede).
- [ ] **Step 6:** Commit.

### Task 5: HUD e controles (React)

**Files:**
- Create: `app/src/live/LiveMatch.tsx`, `app/src/live/LiveMatch.test.tsx`

**Interfaces:**
- Consumes: `createPlayback`, `buildChoreography`, `PitchView`.
- Produces: `<LiveMatch timeline />` com placar, minuto, faixa de narração (`aria-live="polite"`), botões Pausar/Retomar, 1x/2x/4x, Pular para o fim.

- [ ] **Step 1:** Testes (Testing Library, `PitchView` substituído por um stub apenas neste teste): Espaço pausa; `1`, `2`, `4` trocam a velocidade; "Pular para o fim" mostra `match.final`; gol atualiza o placar só quando o lance de gol é alcançado; sem `commentary` não aparece "undefined"; foco visível nos botões.
- [ ] **Step 2:** `npm test -- LiveMatch`. Expected: FAIL.
- [ ] **Step 3:** Implementar `LiveMatch.tsx`.
- [ ] **Step 4:** `npm test -- LiveMatch`. Expected: PASS.
- [ ] **Step 5:** Commit.

### Task 6: Camada de análise (visual C)

**Files:**
- Create: `app/src/live/overlay.ts`, `app/src/live/overlay.test.ts`; Modify: `PitchView.tsx`, `LiveMatch.tsx`

**Interfaces:**
- Produces: `buildOverlay(t: Timeline, upToIndex: number): { heat: number[][]; links: Array<{ from: number; to: number; weight: number }>; possession: [number, number] }` calculado só com os lances já ocorridos (contagem por área e por par de jogadores citados). Botões ligam/desligam: nomes, rede de passes, calor. Todos rotulados "aproximação visual".

- [ ] **Step 1:** Testes: com `upToIndex = 0` tudo zerado; os pesos só crescem com o índice; a posse soma 100%; a soma do calor bate com a contagem de lances; mesma entrada, mesma saída.
- [ ] **Step 2:** `npm test -- overlay`. Expected: FAIL.
- [ ] **Step 3:** Implementar e ligar à interface (desligada por padrão).
- [ ] **Step 4:** `npm test` e `npm run build`. Expected: PASS.
- [ ] **Step 5:** Commit.

### Task 7: Integração, CI, documentação e revisão

**Files:**
- Modify: `app/src/App.tsx` (depois de simular, botão "Assistir partida" abre `LiveMatch`; a lista de lances vira alternativa), `.github/workflows/app.yml` se necessário; `README.md`, `ROADMAP_BYGFOOT_MODERNO.md`
- Create: `docs/adr/ADR-004-reproducao-2d-e-coreografia.md`

- [ ] **Step 1:** Teste de integração em `App.test.tsx`: simula (mock) e assiste até o fim; o placar exibido é `match.final`.
- [ ] **Step 2:** Implementar a navegação; rodar `npm test`, `npm run typecheck`, `npm run build` e `cargo test --workspace` com `BYGFOOT_BIN`. Expected: tudo PASS.
- [ ] **Step 3:** Empurrar a branch e conferir o CI nos 3 sistemas. Expected: verde.
- [ ] **Step 4:** ADR-004 (decisões, limites: sem coordenadas, medição de fps só em máquina de desenvolvimento, lances gerados de uma vez e não lance a lance).
- [ ] **Step 5:** Revisão independente da branch por um agente separado (modelo mais capaz); corrigir Críticos e Importantes com teste; registrar Menores.
- [ ] **Step 6:** Pedir confirmação ao usuário antes do merge em `main`.

## Fora do escopo (decisão registrada)

- Simulação espacial real (coordenadas do motor) fica depois do v0.1.
- Geração lance a lance em tempo real pelo motor: hoje a partida inteira é gerada antes e tocada em ritmo de jogo. O resultado visual é o mesmo; o limite é registrado na ADR-004.
- Telas de gestão (Fase 4) e empacotamento do motor (Fase 6).
