# Fase 3: desempenho e verificação visual do campo

Data: 2026-11-03. Vitrine de desenvolvimento (`app/live-demo.html`), PixiJS 8.22.0, Chrome no macOS.

## O que foi medido
- Partida de fixture (seed 1) tocando em 4x por 10 s, com `requestAnimationFrame`:
  - 599 quadros, mediana **16,7 ms**, p95 18,6 ms, p99 18,7 ms, máximo 33,3 ms, 1 quadro acima de 20 ms.
  - Canvas de 1520×984 (tela retina, devicePixelRatio 2). Console sem erros nem avisos.
- Layout na **vitrine** (sem o formulário do app): com viewport de 1470×741, o campo (820×531) e os controles cabem na altura, sem rolagem horizontal. No app o formulário fica acima do campo, que pode exigir rolar a página; isso não foi medido.

## Limites desta medição
- O monitor é de 60 Hz, então 16,7 ms é o quadro do vsync: mostra que não houve lentidão, mas não prova folga acima de 60 fps.
- A altura do viewport ficou travada em 741 px (limite do monitor), mesmo pedindo 768 e 1080 de altura: **1366×768 e 1920×1080 não foram validados de fato**. A largura chegou a 1920 px: com viewport 1920×741 o campo (820×531) e os controles cabem (controles terminam em 721 px), sem rolagem horizontal. Tamanhos maiores dependem só da conta de layout (largura do campo limitada pela altura da janela).
- O campo acompanha o contêiner sem a janela mudar: encolhendo só o contêiner de 820 para 520 px, o canvas foi para 520×337 mantendo a proporção.
- Só o tema escuro do sistema foi visto; o claro depende dos tokens testados em `tokens.test.ts`, sem captura.
- Chrome no macOS. O WebView do Tauri no Windows (WebView2) e no Linux (WebKitGTK) não foi medido; o CI não mede desempenho.
- Movimento reduzido: coberto por teste de unidade (`lerpFrame` com `reducedMotion`), não visto na tela.
