import { createPlayback } from "./playback";
import type { Unit } from "../types";

const u = (minute: number, time = 0): Unit => ({
  minute, time, possession: 0, area: 1, result: [0, 0], event: { type: 0, team: 0 },
});
// 4 s por minuto de jogo => 1 minuto = 4000 ms a 1x
const opts = { secondsPerMinute: 4 };

test("avança conforme os minutos e escolhe o lance correto", () => {
  const p = createPlayback([u(0), u(10), u(20)], opts);
  p.play();
  p.tick(200); // dentro do limite por tick
  expect(p.state().index).toBe(0);
  for (let i = 0; i < 200; i++) p.tick(200); // 40 s => minuto 10
  expect(p.state().index).toBe(1);
});

test("a velocidade multiplica o avanço", () => {
  const a = createPlayback([u(0), u(30)], opts);
  const b = createPlayback([u(0), u(30)], opts);
  a.play(); b.play(); b.setSpeed(4);
  for (let i = 0; i < 20; i++) { a.tick(100); b.tick(100); }
  expect(b.state().elapsedMs).toBeCloseTo(a.state().elapsedMs * 4, 5);
});

test("pausar congela o relógio", () => {
  const p = createPlayback([u(0), u(30)], opts);
  p.play(); p.tick(100); p.pause();
  const before = p.state().elapsedMs;
  p.tick(100);
  expect(p.state().elapsedMs).toBe(before);
  expect(p.state().playing).toBe(false);
});

test("minuto -1 herda o minuto anterior e o minuto exibido nunca diminui", () => {
  const p = createPlayback([u(0), u(10), u(-1), u(-1), u(12)], opts);
  p.play();
  let last = -Infinity;
  while (!p.state().finished) {
    p.tick(100);
    const m = p.state().minute;
    expect(m).toBeGreaterThanOrEqual(last);
    last = m;
  }
  expect(last).toBe(12);
});

test("pênaltis (time 3) usam duração fixa por lance, sem inventar minutos", () => {
  const p = createPlayback([u(120, 2), u(-1, 3), u(-1, 3)], opts);
  p.play();
  let ticks = 0;
  while (!p.state().finished && ticks < 100000) { p.tick(100); ticks++; }
  expect(ticks).toBeLessThan(100000);
  // 2 lances de pênalti x 1500 ms, exatamente
  expect(p.state().elapsedMs).toBe(3000);
  expect(p.state().minute).toBe(120);
});

test("um tick gigante (aba em segundo plano) é limitado a 250 ms", () => {
  const p = createPlayback([u(0), u(90)], opts);
  p.play(); p.tick(10_000_000);
  expect(p.state().elapsedMs).toBe(250);
});

test("pular para o fim termina no último lance e para o relógio", () => {
  const p = createPlayback([u(0), u(45), u(90)], opts);
  p.play(); p.seekToEnd();
  const s = p.state();
  expect(s.index).toBe(2);
  expect(s.finished).toBe(true);
  expect(s.playing).toBe(false);
});

test("com um único lance, a partida já está no fim e não quebra", () => {
  const p = createPlayback([u(0)], opts);
  p.play(); p.tick(100);
  expect(p.state()).toMatchObject({ index: 0, finished: true, progress: 0 });
});

test("lista vazia é recusada", () => {
  expect(() => createPlayback([], opts)).toThrow();
});

test("o índice nunca diminui, com ticks irregulares e trocas de velocidade", () => {
  const p = createPlayback([u(0), u(5), u(5), u(-1), u(40), u(90)], opts);
  p.play();
  let prev = 0;
  for (let i = 0; i < 3000; i++) {
    if (i % 97 === 0) p.setSpeed(([1, 2, 4] as const)[i % 3]);
    if (i % 211 === 0) { p.pause(); p.play(); }
    p.tick((i * 37) % 300);
    expect(p.state().index).toBeGreaterThanOrEqual(prev);
    prev = p.state().index;
  }
});

test("restart volta ao início", () => {
  const p = createPlayback([u(0), u(10)], opts);
  p.play(); p.seekToEnd(); p.restart();
  expect(p.state()).toMatchObject({ index: 0, finished: false, elapsedMs: 0 });
});

test("nenhum lance é pulado, nem no maior avanço possível por tick (250 ms a 4x)", () => {
  // lances colados no mesmo minuto: o intervalo mínimo entre eles é o que protege os anúncios de gol
  const p = createPlayback([u(0), u(0), u(0), u(1), u(1), u(1), u(2)], opts);
  p.play(); p.setSpeed(4);
  let prev = 0, guard = 0;
  while (!p.state().finished && guard++ < 10000) {
    p.tick(250);
    const i = p.state().index;
    expect(i - prev).toBeLessThanOrEqual(1);
    prev = i;
  }
  expect(guard).toBeLessThan(10000);
  expect(prev).toBe(6);
});
