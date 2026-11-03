# Design system: partida ao vivo (Fase 3)

Fonte de verdade dos tokens: `app/src/design/tokens.ts` (testes de contraste em `tokens.test.ts`).
Visual escolhido pelo usuário: **B (transmissão)** como base + **C (análise)** como camada opcional.
Protótipos: https://claude.ai/artifact/NgQ2BcDUu1f6JpSDD7QGYp. Todo time, camisa e nome é fictício.

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
Nomes `#f2f8fb`, rede de passes e calor `#6fe0c0`. Desligada por padrão. Tudo é **aproximação visual**: o motor não fornece coordenadas, então rede e calor são contagens por área e por par de jogadores citados nos lances.

## Controles (tema claro/escuro da página)
Claro: fundo `#f3f4f1`, texto `#151a17`, destaque `#0b6e4f`. Escuro: fundo `#101411`, texto `#e8ece6`, destaque `#5fd0a0`. Texto e botão ≥ 4,5:1.

## Estados
- **Vazio:** "Nenhuma partida simulada ainda."
- **Carregando:** botão "Simulando…" desabilitado.
- **Erro:** alerta com a mensagem do adaptador, formulário continua usável.
- **Fim de jogo:** placar final fixo e botão "Rever".

## Acessibilidade
Foco visível, atalhos de teclado (Espaço pausa; 1, 2 e 4 trocam a velocidade; Fim pula para o final), `aria-live="polite"` na narração, `prefers-reduced-motion` desliga a interpolação (os lances aparecem por troca de estado). Validar em 1366×768 e 1920×1080.
