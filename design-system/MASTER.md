# Design system: partida ao vivo (Fase 3)

Fonte de verdade dos tokens: `app/src/design/tokens.ts` (testes de contraste em `tokens.test.ts`).
Visual escolhido pelo usuário: **B (transmissão)** como base + **C (análise)** como camada opcional.
As cores das camisas são fictícias. Os nomes de times e jogadores exibidos vêm do banco de dados do upstream (inclui nomes de clubes reais das Ilhas Faroé): trocar por um dataset fictício é pendência da regra 7 do CLAUDE.md, antes de qualquer distribuição.

## Campo (B)
| Token | Valor | Uso |
|---|---|---|
| `pitch.grassA` / `grassB` | `#2a8647` / `#34995a` | faixas da grama |
| `pitch.line` | `#f4fbf5` | linhas do campo |
| `kits.home` | camisa `#1b3fa0`, borda `#ffffff` | time da casa |
| `kits.away` | camisa `#ffd23f`, borda `#111111` | time de fora |

Os dois kits diferem em luminosidade (não só em matiz), para continuarem distintos em daltonismo. Cada camisa tem borda com contraste mínimo de 3:1 contra a grama.

## Placar e narração (B)
Fundo `#10151c`, texto `#ffffff`, destaque `#ffd23f` (≥ 4,5:1). Fonte de display condensada para o placar, fonte de texto legível para a narração.

## Camada de análise (C)
Nomes `#f2f8fb` e calor `#6fe0c0`. Desligada por padrão. Tudo é **aproximação visual**: o motor não fornece coordenadas. O calor é a contagem de lances por zona do campo; "Combinações" lista pares de jogadores citados juntos em lances de jogo corrido (tipo 0); "Nomes" mostra os jogadores do lance atual. Não há rede desenhada entre pontos do campo.

## Controles (tema claro/escuro da página)
Os tokens testados estão em `tokens.chrome` (claro: texto `#151a17` sobre `#f3f4f1`; escuro: `#e8ece6` sobre `#101411`; botão de destaque `#0b6e4f` / `#5fd0a0`). O `styles.css` publicado usa fundos um pouco diferentes (`#f6f7f9`, `#14161a`, cartão `#1e2128`) com os mesmos botões de destaque; o teste de contraste cobre os tokens, não o CSS publicado.

## Estados
- **Vazio:** "Nenhuma partida simulada ainda."
- **Carregando:** botão "Simulando…" desabilitado.
- **Erro:** alerta com a mensagem do adaptador, formulário continua usável.
- **Fim de jogo:** placar final fixo e botão "Rever".

## Acessibilidade
Foco visível, atalhos de teclado (Espaço pausa; 1, 2 e 4 trocam a velocidade; Fim pula para o final), `aria-live="polite"` só nos lances importantes (início, gol, cartões, pênalti, intervalo e fim), com o placar; a legenda visual não é anunciada a cada lance, `prefers-reduced-motion` desliga a interpolação (os lances aparecem por troca de estado). Validar em 1366×768 e 1920×1080.
