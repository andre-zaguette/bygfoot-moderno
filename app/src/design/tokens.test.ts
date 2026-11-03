import { contrast, tokens } from "./tokens";

const { pitch, kits, hud, overlay, chrome } = tokens;

test("contrast: preto/branco = 21 e igual consigo mesmo = 1", () => {
  expect(contrast("#000000", "#ffffff")).toBeCloseTo(21, 0);
  expect(contrast("#336699", "#336699")).toBeCloseTo(1, 5);
});

test("os dois kits se distinguem entre si (>= 3:1)", () => {
  expect(contrast(kits.home.fill, kits.away.fill)).toBeGreaterThanOrEqual(3);
});

test("a borda de cada camisa contrasta com as duas faixas da grama (>= 3:1)", () => {
  for (const k of [kits.home, kits.away]) {
    for (const g of [pitch.grassA, pitch.grassB]) {
      expect(contrast(k.border, g)).toBeGreaterThanOrEqual(3);
    }
  }
});

test("cada camisa contrasta com a própria borda (>= 3:1)", () => {
  for (const k of [kits.home, kits.away]) expect(contrast(k.fill, k.border)).toBeGreaterThanOrEqual(3);
});

test("linhas do campo e nomes da camada de análise são legíveis sobre a grama (>= 3:1)", () => {
  for (const g of [pitch.grassA, pitch.grassB]) {
    expect(contrast(pitch.line, g)).toBeGreaterThanOrEqual(3);
    expect(contrast(overlay.name, g)).toBeGreaterThanOrEqual(3);
  }
});

test("texto do placar contrasta com o fundo (>= 4.5:1), inclusive o destaque", () => {
  expect(contrast(hud.fg, hud.bg)).toBeGreaterThanOrEqual(4.5);
  expect(contrast(hud.accent, hud.bg)).toBeGreaterThanOrEqual(4.5);
});

test("controles em tema claro e escuro: texto (>= 4.5:1) e botão de destaque (>= 4.5:1)", () => {
  for (const t of [chrome.light, chrome.dark]) {
    expect(contrast(t.fg, t.bg)).toBeGreaterThanOrEqual(4.5);
    expect(contrast(t.accentFg, t.accent)).toBeGreaterThanOrEqual(4.5);
  }
});
