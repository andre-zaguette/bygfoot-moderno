# ADR-001: origem do código de simulação (Caminho A ou B)

- Data: 2026-11-01
- Estado: **proposta para aprovação da pessoa responsável** (atualizado ao fim da Fase 1, 2026-11-02)
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

**Resultado do G1 após a Fase 1: cumprido nos pontos testados, com ressalvas.** Simular sem janela com resultado repetível funciona em macOS, Linux e Windows (CI). A igualdade ENTRE sistemas **não vem de graça**: o upstream original diverge entre eles em 6 de 6 seeds. Com 3 remendos (`patches/0001..0003`, ver [ADR-002](ADR-002-determinismo-entre-sistemas.md)) os três sistemas dão a mesma timeline de partida (6 seeds de ouro e 16 seeds por 30 semanas; o hash cobre o documento `timeline-v1` completo). "Dados versionados" passou a existir como contrato nosso: `MatchTimeline v1` (`schemas/match-timeline-v1.schema.json`, teste de contrato nos 3 sistemas, execução 41481640595); o save do upstream continua sem versão. A decisão acima se mantém.

## O que o G1 NÃO provou (condições para manter a decisão)

Note: "dados versionados" aparece aqui como ausência no upstream. Ainda não foi testado que a nossa camada consegue acrescentar essa versão por cima; isso é trabalho da Fase 1.

| Condição | Estado | Se falhar |
|---|---|---|
| Rodar em Windows e Linux | **verificado no CI** (macOS, Ubuntu e Windows via MSYS2/MinGW64): compila e passa os testes. **Resultados idênticos entre sistemas: só com os remendos 0001..0003** (ADR-002); sem eles divergem | manter o CI verde e as baterias de seeds nos 3 sistemas |
| Dados versionados | **contrato `MatchTimeline v1` criado e testado** (versão no documento). O save do upstream continua sem versão (`load_save.c:206`, sem `PRAGMA user_version`); versionar o save fica para a Fase 2 | o `SimulationAdapter` versiona por cima; o formato interno do upstream continua sendo dele |
| Lances em tempo real | os lances saem todos juntos no fim; a hipótese de geração lance a lance não foi verificada em execução | a animação usa a linha do tempo completa; pausar e mexer na tática no meio do jogo (Fase 3) pode exigir um remendo ou outro desenho |
| Partida avulsa por ids | o motor simula o calendário de um país; só a última partida do usuário guarda os lances | remendo no upstream ou "gravar lances" para qualquer partida |
| Licença | upstream tem 178 arquivos GPL-2.0-or-later e 23 sem cabeçalho (`docs/LEGAL_REVIEW.md`) | decisão jurídica antes de distribuir |

## Consequências

- Remendos no upstream: **3 aplicados** (`patches/0001..0003`, todos de determinismo entre sistemas, ADR-002) sobre uma cópia; `upstream/` segue intacto. **Candidatos ainda não aplicados**: 7 (`docs/spikes/headless-result.md`): locale do JSON, `--random-seed` com sinal, busca de `support_files`/`-s`, lances de qualquer partida, comando de exportação de lances, avisos de comentário, saída em `stdout.log` no Windows. Os três primeiros já têm contorno no wrapper.
- Atualizar o upstream tem processo e medida: `tools/sync-upstream.sh` e `.github/workflows/sync-upstream.yml`. Entre a `3.0.0` e o commit fixado (10 commits) as 6 seeds de ouro não mudaram ([sync-experiment](../spikes/sync-experiment.md)).
- Limites conhecidos do determinismo (ADR-002): cobertura só dos caminhos exercitados, `powf` com 1 ulp de diferença entre bibliotecas matemáticas. Replays da timeline são iguais entre sistemas com os remendos nas seeds testadas; saves completos não foram comparados (há diferença conhecida em `value`/`wage`).
- O binário original continua ligado ao GTK. O produto empacotado carregaria as bibliotecas GTK, a menos que a Fase 1 extraia uma biblioteca. Isso pesa no instalador de Windows e macOS.
- O pós-processamento em Python do spike é descartável; o produto fará em Rust.

## Alternativas consideradas

- **Extrair `libbygfoot_core` sem GTK agora:** mais trabalho, sem evidência de que seja necessário, já que o console funciona. Adiada.
- **Motor próprio em Rust (Caminho B):** descarta o código existente e o conteúdo de regras do Bygfoot. Só se o G1 falhar nas condições acima.
