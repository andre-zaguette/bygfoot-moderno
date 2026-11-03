# ADR-004: Reprodução 2D como coreografia sobre a timeline do motor

Status: aceita (Fase 3, 2026-11-03)

## Decisão
A partida ao vivo em 2D é a **reprodução** da `MatchTimeline v1` já produzida pelo motor, em três camadas independentes:
1. `app/src/live/playback.ts`: relógio puro (pausa, 1x/2x/4x, pular para o fim). 1x = 4 s reais por minuto de jogo. Lance com `minute: -1` herda o minuto anterior, pênaltis têm duração fixa e um salto grande de tempo (aba em segundo plano) é limitado a 250 ms de relógio real por tick (até 1000 ms de tempo de jogo a 4x). Com o intervalo mínimo entre lances, nenhum lance é pulado nem nesse limite (teste de regressão em `playback.test.ts`).
2. `app/src/live/choreography.ts`: função pura e determinística que converte cada lance (área, posse, tipo) em posições de bola e dos 22 jogadores. O PRNG é derivado de `source.seed` e do índice do lance. O placar de cada quadro é **copiado** do lance; nada calcula resultado.
3. `app/src/live/PitchView.tsx`: renderer PixiJS 8.22.0 que só desenha o quadro atual (interpolação suavizada, smoothstep, entre lances; com movimento reduzido, sem posições intermediárias).

Visual: transmissão (B) como base e camada de análise (C) opcional e desligada por padrão. Tokens e contrastes testados em `design-system/MASTER.md` e `app/src/design/tokens.test.ts`.

## Camada de análise: o que ela mostra e o que não mostra
O motor informa área, posse e jogadores citados, **não a posição de cada jogador**. Por isso:
- calor por zona do campo e posse em %: contagens reais por lance;
- "Combinações": pares de jogadores citados juntos em lances de jogo corrido (tipo 0, em geral passes). Gol, falta, cartão e substituição ficam de fora, porque nesses lances o segundo jogador é o goleiro, o faltoso, a vítima ou quem sai. É contagem real, sem desenhar linhas entre pontos do campo;
- nomes do lance atual numa legenda, sem colar nomes em discos.
Tudo rotulado como aproximação visual (regra 3 do projeto).

## Limites conhecidos
- **Não é geração ao vivo.** O motor produz a partida inteira antes e o app a toca em ritmo de jogo. Para quem assiste parece ao vivo; geração lance a lance exigiria mudar o motor e fica fora do escopo.
- **Sem coordenadas:** a posição dos jogadores é coreografia (4-4-2 fixo deslocado por área e posse). Dois jogadores podem se sobrepor.
- **Desempenho medido só no Chrome/macOS** (ver `docs/spikes/fase-3-desempenho.md`): mediana de 16,7 ms por quadro numa tela de 60 Hz. WebView2 (Windows) e WebKitGTK (Linux) não foram medidos; o CI não mede desempenho.
- 1366×768 e 1920×1080 não foram validados na tela (a janela não passou de ~1470×741); a largura do campo é limitada pela altura da janela por CSS.
- **Acessibilidade:** foco e atalhos de teclado funcionam ao abrir, pular e voltar; leitores de tela recebem só os lances importantes, com o placar. Os comentários do motor estão em inglês (`lang="en"`) numa interface em português: tradução fica para depois.
- Os fixtures de teste (`app/src/live/__fixtures__/`) usam nomes de times e jogadores do banco de dados do upstream; nenhum asset real de terceiros é usado pela interface.
- A vitrine `app/live-demo.html` serve só para desenvolvimento e não entra no build.
