# CLAUDE.md — Football Manager 2D (modernização do Bygfoot)

## Fonte de verdade

Leia primeiro `ROADMAP_BYGFOOT_MODERNO.md`. Para implementação, leia também `docs/GDD.md`, `docs/ARCHITECTURE.md` e ADRs relevantes. Se algum arquivo ainda não existir, crie somente quando a tarefa pertinente exigir. Nunca invente estado de progresso.

## Produto

Gerenciador de futebol offline-first para desktop, com interface moderna e partida animada em 2D. O MVP deve permitir concluir uma temporada de 12 clubes e 22 rodadas, com elenco, escalação, tática, transferências básicas, calendário, classificação, save/load e replay de partidas.

## Stack proposta (dependente do gate G1)

- React, TypeScript, Vite, Tailwind CSS e componentes acessíveis.
- Tauri v2 / Rust para a camada local, SQLite para persistência.
- PixiJS 8 para o renderer de partidas 2D.
- Motor de simulação atrás de `SimulationAdapter` versionado.
- Reutilizar Bygfoot C somente se for possível extrair execução headless e cumprir as obrigações de licença. Não misturar diretamente código GPL-2.0-only e GPLv3 sem revisão.
- Godot é alternativa arquitetural, não dependência do MVP.

## Regras técnicas inegociáveis

1. **Não começar reescrevendo todo o motor.** Investigar o Bygfoot 3.0.0 e executar spike técnico.
2. O renderer 2D **não calcula resultado**. Ele representa `MatchTimeline` já produzida pela simulação.
3. Se o motor não gerar coordenadas, nomear o replay como **aproximação visual**, não simulação física.
4. Mesma seed + estado + versão = mesmo resultado, salvo decisão de projeto documentada.
5. Todos os DTOs e schemas de save/eventos têm versões e testes de contrato.
6. Nenhuma query de UI modifica SQLite sem passar pela camada de aplicação.
7. Não usar escudos, logos, nomes e imagens de atletas reais sem direitos apropriados; priorizar dataset fictício.
8. Não inserir pacotes/scripts/hook de skills automaticamente sem inspecionar origem, conteúdo, permissões, dependências e licença.
9. Não afirmar que algo foi executado, validado ou aprovado sem evidências (comando, logs, testes, screenshot).
10. Não adicionar 3D, multiplayer, cloud, monetização ou AI gerativa ao MVP sem ADR e replanejamento.

## Política de UI/UX

- Interfaces densas mas legíveis, inspiradas em uma central de análise de futebol; evitar aparência genérica de dashboard SaaS.
- Antes de codificar UI: propostas visuais, tokens em `design-system/MASTER.md`, wireframe e estados de erro/vazio/loading.
- Foco visível, navegação por teclado, alto contraste e movimento reduzido; alvo WCAG 2.2 AA pertinente.
- Validar telas em 1366×768 e 1920×1080 desde o protótipo.
- UI/UX Pro Max e Frontend Design podem orientar a proposta, mas os tokens aprovados são a fonte de verdade.

## Skills por contexto (usar somente se instaladas)

- GDD/escopo: `game-vision-architect`, `core-loop-designer`, `prototype-scope-definer`, `gdd-author`, `scope-feature-prioritizer`.
- Regras e economia: `rules-formalizer`, `economy-progression-designer`, `game-balance-analyst`.
- UX: `ui-ux-systems-designer`, `game-ui-ux`, `ui-ux-pro-max`, `frontend-design`.
- Campo: `pixijs-rendering`, `create-game-assets`, `game-feel`.
- Qualidade: `save-systems`, `react-best-practices`, `web-design-guidelines`, e práticas Superpowers para TDD/debug/review.

## Procedimento para cada issue

1. Ler requisitos, dependências, contratos, ADR e critérios de aceite.
2. Propor plano curto com arquivos a editar, riscos e testes.
3. Implementar o menor incremento completo; evitar mudanças não relacionadas.
4. Executar testes relevantes, lint, typecheck e build; registrar comandos e resultados.
5. Para simulação, testar invariantes e golden seeds. Para UI, testar estados e teclado.
6. Revisar segurança, licenças, mudanças de schema e atualizações de documentação.
7. Só marcar como concluído com critérios demonstrados e sem regressões conhecidas.

## Comportamento esperado

- Prefira dados fictícios e seeds fixas para demos.
- Registre decisões significativas em `docs/adr/`.
- Não crie rotinas de rede, analytics ou upload de saves por padrão.
- Seja claro sobre hipóteses versus código existente no Bygfoot.
- Se o gate G1 falhar, documente bloqueios reproduzíveis, recomende o pivot Rust e atualize o roadmap.

## Primeira tarefa

Executar engenharia reversa do Bygfoot 3.0.0 em modo de leitura, localizar as funções de simulação e as dependências GTK, identificar os dados SQLite e produzir `docs/REVERSE_ENGINEERING.md`, `docs/LEGAL_REVIEW.md` e o plano do spike headless. **Não alterar o upstream antes do diagnóstico.**
