// Tokens da partida ao vivo (visual B "transmissão" + camada C "análise").
// Fonte de verdade documentada em design-system/MASTER.md. Cores das camisas definidas aqui.
export const tokens = {
  pitch: { grassA: "#2a8647", grassB: "#34995a", line: "#f4fbf5" },
  kits: {
    home: { fill: "#1b3fa0", border: "#ffffff" },
    away: { fill: "#ffd23f", border: "#111111" },
  },
  hud: { bg: "#10151c", fg: "#ffffff", accent: "#ffd23f" },
  overlay: { name: "#f2f8fb", link: "#6fe0c0", heat: "#6fe0c0" },
  chrome: {
    light: { bg: "#f3f4f1", fg: "#151a17", accent: "#0b6e4f", accentFg: "#ffffff" },
    dark: { bg: "#101411", fg: "#e8ece6", accent: "#5fd0a0", accentFg: "#06251a" },
  },
} as const;

function luminance(hex: string): number {
  const n = parseInt(hex.replace("#", ""), 16);
  const ch = [(n >> 16) & 255, (n >> 8) & 255, n & 255].map((v) => {
    const s = v / 255;
    return s <= 0.03928 ? s / 12.92 : Math.pow((s + 0.055) / 1.055, 2.4);
  });
  return 0.2126 * ch[0] + 0.7152 * ch[1] + 0.0722 * ch[2];
}

/** Razão de contraste WCAG entre duas cores #rrggbb (1 a 21). */
export function contrast(a: string, b: string): number {
  const [hi, lo] = [luminance(a), luminance(b)].sort((x, y) => y - x);
  return (hi + 0.05) / (lo + 0.05);
}
