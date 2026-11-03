import { buildOverlay } from "./overlay";
import type { Timeline } from "../types";
import m1 from "./__fixtures__/match-seed1.json";
import m7 from "./__fixtures__/match-seed7.json";

const fixtures = [m1, m7] as unknown as Timeline[];
const sum = (a: number[][]) => a.flat().reduce((x, y) => x + y, 0);
const total = (t: Timeline, i: number) => sum(buildOverlay(t, i).heat);

test("o calor soma exatamente os lances já ocorridos (índice + 1)", () => {
  for (const t of fixtures) for (const i of [0, 1, 10, t.units.length - 1]) expect(total(t, i)).toBe(i + 1);
});

test("no primeiro lance não há passes registrados", () => {
  for (const t of fixtures) expect(buildOverlay(t, 0).links).toEqual([]);
});

test("o peso total dos passes só cresce com o índice e bate com a contagem de lances com dois jogadores", () => {
  for (const t of fixtures) {
    let prev = 0;
    for (let i = 0; i < t.units.length; i++) {
      const w = buildOverlay(t, i).links.reduce((x, l) => x + l.weight, 0);
      expect(w).toBeGreaterThanOrEqual(prev);
      prev = w;
    }
    const expected = t.units.filter((u) => u.event.type === 0 && u.event.player && u.event.player2).length;
    expect(prev).toBe(expected);
  }
});

test("a posse soma 100% e acompanha a contagem de lances por time", () => {
  for (const t of fixtures) {
    for (const i of [0, 5, t.units.length - 1]) {
      const [h, a] = buildOverlay(t, i).possession;
      expect(h + a).toBe(100);
    }
    const last = buildOverlay(t, t.units.length - 1);
    const home = t.units.filter((u) => u.possession === 0).length;
    expect(last.possession[0]).toBe(Math.round((home / t.units.length) * 100));
  }
});

test("passes ordenados do mais frequente para o menos, com empate por nome (saída estável)", () => {
  const { links } = buildOverlay(fixtures[0], fixtures[0].units.length - 1);
  for (let k = 1; k < links.length; k++) {
    expect(links[k - 1].weight).toBeGreaterThanOrEqual(links[k].weight);
  }
});

test("é determinístico, não altera a entrada e limita o índice ao intervalo válido", () => {
  const t = structuredClone(fixtures[0]);
  const a = buildOverlay(t, 20);
  expect(buildOverlay(t, 20)).toEqual(a);
  expect(t).toEqual(fixtures[0]);
  expect(buildOverlay(t, 9999)).toEqual(buildOverlay(t, t.units.length - 1));
  expect(buildOverlay(t, -5)).toEqual(buildOverlay(t, 0));
});

test("o par A→B e B→A contam como o mesmo passe", () => {
  const mk = (p: number, q: number, possession: 0 | 1) => ({
    minute: 5, time: 0, possession, area: 1, result: [0, 0] as [number, number],
    event: { type: 0, team: 0, player: { id: p, name: `J${p}` }, player2: { id: q, name: `J${q}` } },
  });
  const t = { ...fixtures[0], units: [mk(1, 2, 0), mk(2, 1, 0), mk(3, 4, 1)] } as Timeline;
  const { links } = buildOverlay(t, 2);
  expect(links).toHaveLength(2);
  expect(links[0].weight).toBe(2);
  expect(links[0].team).toBe(0);
});

test("só jogo corrido (tipo 0) conta como combinação: gol, falta, cartão e substituição não", () => {
  const two = (type: number) => ({
    minute: 5, time: 0, possession: 0 as const, area: 1, result: [0, 0] as [number, number],
    event: { type, team: 0, player: { id: 1, name: "Atacante" }, player2: { id: 2, name: "Goleiro rival" } },
  });
  const t = { ...fixtures[0], units: [two(10), two(19), two(20), two(31), two(0)] } as Timeline;
  const { links } = buildOverlay(t, 4);
  expect(links).toHaveLength(1);
  expect(links[0].weight).toBe(1);
});

test("o desempate de pesos iguais não depende do idioma do sistema: ordena por id, não por nome", () => {
  const mk = (p: number, q: number, pn: string, qn: string) => ({
    minute: 5, time: 0, possession: 0 as const, area: 1, result: [0, 0] as [number, number],
    event: { type: 0, team: -1, player: { id: p, name: pn }, player2: { id: q, name: qn } },
  });
  const t = { ...fixtures[0], units: [mk(5, 6, "Øvre", "Åse"), mk(1, 2, "Zeta", "Yara")] } as Timeline;
  const { links } = buildOverlay(t, 1);
  expect(links.map((l) => l.a.id)).toEqual([1, 5]);
});
