import { lerpFrame } from "./interpolate";
import type { Frame } from "./choreography";

const mk = (bx: number, px: number, caption: string, score: [number, number]): Frame => ({
  unitIndex: 0, minute: 10,
  ball: { x: bx, y: 20 },
  players: { home: Array.from({ length: 11 }, () => ({ x: px, y: 10 })), away: Array.from({ length: 11 }, () => ({ x: px + 5, y: 50 })) },
  effect: null, score, caption,
});
const a = mk(10, 20, "A", [0, 0]);
const b = mk(30, 40, "B", [1, 0]);

test("t = 0 devolve as posições de a e t = 1 as de b", () => {
  expect(lerpFrame(a, b, 0).ball).toEqual(a.ball);
  expect(lerpFrame(a, b, 1).ball).toEqual(b.ball);
  expect(lerpFrame(a, b, 1).players.home[3]).toEqual(b.players.home[3]);
});

test("t = 0,5 fica no meio (bola e jogadores)", () => {
  const f = lerpFrame(a, b, 0.5);
  expect(f.ball.x).toBeCloseTo(20);
  expect(f.players.home[0].x).toBeCloseTo(30);
  expect(f.players.away[10].x).toBeCloseTo(35);
});

test("placar, legenda e efeito vêm do lance atual (a), nunca do próximo", () => {
  const f = lerpFrame(a, b, 0.9);
  expect(f.score).toEqual([0, 0]);
  expect(f.caption).toBe("A");
});

test("t fora de 0..1 é limitado", () => {
  expect(lerpFrame(a, b, -3).ball.x).toBe(10);
  expect(lerpFrame(a, b, 7).ball.x).toBe(30);
});

test("movimento reduzido: sempre o quadro atual, sem posições intermediárias", () => {
  for (const t of [0, 0.3, 0.99, 1]) expect(lerpFrame(a, b, t, true)).toEqual(a);
});

test("posições interpoladas continuam dentro do campo e a entrada não é alterada", () => {
  const snap = structuredClone([a, b]);
  for (let t = 0; t <= 1; t += 0.1) {
    const f = lerpFrame(a, b, t);
    for (const v of [f.ball, ...f.players.home, ...f.players.away]) {
      expect(v.x).toBeGreaterThanOrEqual(0); expect(v.x).toBeLessThanOrEqual(105);
      expect(v.y).toBeGreaterThanOrEqual(0); expect(v.y).toBeLessThanOrEqual(68);
    }
  }
  expect([a, b]).toEqual(snap);
});
