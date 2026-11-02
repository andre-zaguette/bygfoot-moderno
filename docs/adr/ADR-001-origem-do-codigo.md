# ADR-001: origem do código de simulação (Caminho A ou B)

- Data: 2026-11-01
- Estado: **proposta para aprovação da pessoa responsável**
- Upstream avaliado: `9b7b0062` (ver `docs/spikes/upstream-pin.md`)

## Contexto

O projeto quer um fork do Bygfoot com interface nova e partida em 2D, mantendo o fork atualizável. O roadmap define o gate G1: provar que o motor simula uma partida sem janela GTK, com dados versionados. Se falhar, pivotar para um motor em Rust (Caminho B).

## Evidências

Tudo rodado em macOS arm64 nesta sessão (detalhes em `docs/spikes/`):

1. O Bygfoot original compila com cmake 4.4.5 e GTK 3.24.52 (`docs/spikes/build-macos.md`).
2. O upstream **já tem um frontend de console**: `bygfoot --json ARQUIVO` roda comandos (`add_user`, `start_bygfoot`, `simulate_games`, `dump_bygfoot`, `save_bygfoot`…) e sai, sem `gtk_init`.
3. Esse binário, sem nenhuma edição em `upstream/`, simulou partidas **sem display**, e o `dump_bygfoot` traz os `LiveGameUnit` da última partida do time do usuário: 121 lances com posse, área, minuto, placar, evento, jogadores e comentário.
4. Determinismo: mesma seed gera saída idêntica byte a byte (`tests/spike/test-headless.sh`), inclusive de pastas diferentes e com duas execuções simultâneas.
5. O núcleo é pouco acoplado: 56 dos 201 arquivos usam a API do GTK, e quase todos são de interface (`docs/REVERSE_ENGINEERING.md`, seção 2).

## Decisão

**Seguir com o Caminho A (fork do Bygfoot), condicionado às condições da tabela abaixo.** Na Fase 1, a primeira integração é o **frontend de console existente, como processo separado conversando por JSON**, em vez de extrair uma biblioteca C sem GTK. A extração de biblioteca fica como opção para depois, se a medição mostrar que o processo separado não basta.

O Caminho B (motor em Rust) continua como plano B, acionado se uma das condições abaixo falhar.

**Resultado do G1: parcial. Cumprido para "simular sem janela, com resultado repetível" em macOS, Linux e Windows, medido no CI do GitHub (execução 41377028975: build do upstream, upstream intacto e `tests/spike/test-headless.sh` verdes nos três). Não cumprido: "dados versionados" (o upstream não versiona o save). Não verificado: se o mesmo seed dá o mesmo resultado ENTRE sistemas (cada sistema só foi comparado consigo mesmo). A decisão acima vale enquanto essas condições forem resolvidas na Fase 1.**

## O que o G1 NÃO provou (condições para manter a decisão)

Note: "dados versionados" aparece aqui como ausência no upstream. Ainda não foi testado que a nossa camada consegue acrescentar essa versão por cima; isso é trabalho da Fase 1.

| Condição | Estado | Se falhar |
|---|---|---|
| Rodar em Windows e Linux | **verificado no CI** (macOS, Ubuntu e Windows via MSYS2/MinGW64): compila e passa os testes do spike. Windows precisou de contornos (ver `docs/spikes/headless-result.md`) | manter o CI verde; resultados idênticos entre sistemas ainda não foram comparados |
| Dados versionados | **o save do upstream não declara versão** (a tag de versão do formato antigo está comentada em `load_save.c:206`; não há `PRAGMA user_version`) | o `SimulationAdapter` adiciona versão de contrato por cima; o formato interno do upstream continua sendo dele |
| Lances em tempo real | os lances saem todos juntos no fim; a hipótese de geração lance a lance não foi verificada em execução | a animação usa a linha do tempo completa; pausar e mexer na tática no meio do jogo (Fase 3) pode exigir um remendo ou outro desenho |
| Partida avulsa por ids | o motor simula o calendário de um país; só a última partida do usuário guarda os lances | remendo no upstream ou "gravar lances" para qualquer partida |
| Licença | upstream tem 178 arquivos GPL-2.0-or-later e 23 sem cabeçalho (`docs/LEGAL_REVIEW.md`) | decisão jurídica antes de distribuir |

## Consequências

- Remendos no upstream: **candidatos listados, nenhum aplicado**. São 7 (`docs/spikes/headless-result.md`): locale do JSON, `--random-seed` com sinal, busca de `support_files`/`-s`, lances de qualquer partida, comando de exportação de lances, avisos de comentário, saída em `stdout.log` no Windows. Os três primeiros já têm contorno no wrapper.
- O binário original continua ligado ao GTK. O produto empacotado carregaria as bibliotecas GTK, a menos que a Fase 1 extraia uma biblioteca. Isso pesa no instalador de Windows e macOS.
- O pós-processamento em Python do spike é descartável; o produto fará em Rust.

## Alternativas consideradas

- **Extrair `libbygfoot_core` sem GTK agora:** mais trabalho, sem evidência de que seja necessário, já que o console funciona. Adiada.
- **Motor próprio em Rust (Caminho B):** descarta o código existente e o conteúdo de regras do Bygfoot. Só se o G1 falhar nas condições acima.
