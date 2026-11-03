import { buildChoreography, FIELD_W, FIELD_H } from "./choreography";
import type { Timeline } from "../types";
import m1 from "./__fixtures__/match-seed1.json";
import m7 from "./__fixtures__/match-seed7.json";
import mS from "./__fixtures__/match-seed431199.json";

const fixtures = [m1, m7, mS] as unknown as Timeline[];

function synthetic(units: Timeline["units"], final: [number, number]): Timeline {
  return {
    schemaVersion: 1,
    source: { engine: "bygfoot", upstreamCommit: "a".repeat(40), seed: 5, country: "x", team: "t", weeks: 1, patches: [] },
    match: { fixtureId: 1, home: "Casa", away: "Fora", final },
    units,
  };
}
const unit = (over: Partial<Timeline["units"][0]> & { type?: number }): Timeline["units"][0] => ({
  minute: 10, time: 0, possession: 0, area: 1, result: [0, 0],
  event: { type: over.type ?? 0, team: 0 }, ...over,
} as Timeline["units"][0]);

test("um quadro por lance e determinismo (mesma timeline, mesmos quadros)", () => {
  for (const t of fixtures) {
    const a = buildChoreography(t);
    expect(a).toHaveLength(t.units.length);
    expect(buildChoreography(t)).toEqual(a);
  }
});

test("não altera a entrada", () => {
  const t = structuredClone(fixtures[0]);
  buildChoreography(t);
  expect(t).toEqual(fixtures[0]);
});

test("o placar de cada quadro é exatamente o do motor (a coreografia não calcula placar)", () => {
  for (const t of fixtures) {
    buildChoreography(t).forEach((f, i) => expect(f.score).toEqual(t.units[i].result));
  }
});

test("todas as coordenadas ficam dentro do campo e há 11 + 11 jogadores", () => {
  for (const t of fixtures) {
    for (const f of buildChoreography(t)) {
      expect(f.players.home).toHaveLength(11);
      expect(f.players.away).toHaveLength(11);
      for (const v of [f.ball, ...f.players.home, ...f.players.away]) {
        expect(v.x).toBeGreaterThanOrEqual(0); expect(v.x).toBeLessThanOrEqual(FIELD_W);
        expect(v.y).toBeGreaterThanOrEqual(0); expect(v.y).toBeLessThanOrEqual(FIELD_H);
      }
    }
  }
});

test("a seed da timeline muda as variações (a mesma partida com outra seed não é idêntica)", () => {
  const a = buildChoreography(fixtures[0]);
  const t2 = structuredClone(fixtures[0]); t2.source.seed += 1;
  expect(buildChoreography(t2)).not.toEqual(a);
});

test("gol: efeito 'goal' e bola na baliza do time cujo placar subiu (lido do result do motor)", () => {
  for (const t of fixtures) {
    const frames = buildChoreography(t);
    let prev: [number, number] = [0, 0];
    t.units.forEach((u, i) => {
      if (u.event.type === 10 || u.event.type === 11) {
        expect(frames[i].effect?.kind).toBe("goal");
        const homeScored = u.result[0] > prev[0];
        expect(frames[i].ball.x).toBeCloseTo(homeScored ? FIELD_W : 0, 0);
        expect(frames[i].effect?.team).toBe(homeScored ? 0 : 1);
      }
      prev = u.result;
    });
  }
});

test("gol contra: a bola vai para a baliza do time que ganhou o ponto", () => {
  const t = synthetic([
    unit({ minute: 0, type: 1, result: [0, 0] }),
    unit({ minute: 30, type: 11, possession: 0, area: 2, result: [0, 1] }), // casa chuta, o ponto vai para o de fora
    unit({ minute: 90, type: 4, result: [0, 1] }),
  ], [0, 1]);
  const f = buildChoreography(t);
  expect(f[1].ball.x).toBeCloseTo(0, 0); // baliza da casa (lado esquerdo): o de fora marcou
  expect(f[1].effect).toMatchObject({ kind: "goal", team: 1 });
  expect(f[1].score).toEqual([0, 1]);
});

test("lance sem commentary usa legenda neutra, nunca 'undefined'", () => {
  const t = synthetic([unit({ minute: 0, type: 1 }), unit({ minute: 12, type: 6 }), unit({ minute: 90, type: 4 })], [0, 0]);
  const caps = buildChoreography(t).map((f) => f.caption);
  expect(caps.every((c) => c.length > 0 && !c.includes("undefined"))).toBe(true);
});

test("lance sem comentário usa o comentário do motor quando existe", () => {
  const t = synthetic([unit({ minute: 0, type: 1 }), { ...unit({ minute: 5 }), event: { type: 0, team: 0, commentary: "Bela jogada." } }, unit({ minute: 90, type: 4 })], [0, 0]);
  expect(buildChoreography(t)[1].caption).toBe("Bela jogada.");
});

test("cartões, escanteio, defesa e substituição viram efeitos próprios", () => {
  const kinds = new Set<string>();
  for (const t of fixtures) for (const f of buildChoreography(t)) if (f.effect) kinds.add(f.effect.kind);
  for (const k of ["goal", "corner", "yellow", "red", "substitution"]) expect(kinds).toContain(k);
});

test("a posse empurra o bloco do time que ataca para perto do gol adversário", () => {
  const home = buildChoreography(synthetic([unit({ minute: 0, type: 1 }), unit({ minute: 10, possession: 0, area: 2 }), unit({ minute: 90, type: 4 })], [0, 0]))[1];
  const away = buildChoreography(synthetic([unit({ minute: 0, type: 1 }), unit({ minute: 10, possession: 1, area: 2 }), unit({ minute: 90, type: 4 })], [0, 0]))[1];
  expect(home.ball.x).toBeGreaterThan(FIELD_W / 2);
  expect(away.ball.x).toBeLessThan(FIELD_W / 2);
});

test("prorrogação e pênaltis (time 2 e 3) geram quadros válidos e mantêm o placar do motor", () => {
  const t = synthetic([
    unit({ minute: 0, type: 1 }),
    unit({ minute: 105, time: 2, type: 3, result: [1, 1] }),
    unit({ minute: -1, time: 3, type: 26, possession: 1, result: [1, 1] }),
    unit({ minute: -1, time: 3, type: 0, possession: 0, result: [2, 1] }),
    unit({ minute: 120, time: 3, type: 4, result: [2, 1] }),
  ], [2, 1]);
  const f = buildChoreography(t);
  expect(f).toHaveLength(5);
  f.forEach((fr, i) => expect(fr.score).toEqual(t.units[i].result));
  expect(f[1].effect?.kind).toBe("extratime");
  expect(f[2].effect?.kind).toBe("shootout");
});

test("a legenda diz de que língua é: texto do motor em inglês, texto de reserva em português", () => {
  const t = synthetic([
    unit({ minute: 0, type: 1 }),
    { ...unit({ minute: 5 }), event: { type: 0, team: 0, commentary: "Nice move." } },
    unit({ minute: 90, type: 4 }),
  ], [0, 0]);
  const f = buildChoreography(t);
  expect(f[1].captionLang).toBe("en");
  expect(f[0].captionLang).toBe("pt-BR");
});
