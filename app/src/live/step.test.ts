import { createPlayback } from "./playback";
import { stepFrame } from "./step";
import { buildChoreography } from "./choreography";
import type { Timeline } from "../types";
import m1 from "./__fixtures__/match-seed1.json";

const t = m1 as unknown as Timeline;
const frames = buildChoreography(t);

test("só avisa a interface quando muda o lance, a reprodução, a velocidade ou o fim (não a cada quadro)", () => {
  const pb = createPlayback(t.units);
  pb.play();
  let key = "";
  let notified = 0;
  for (let i = 0; i < 30; i++) {
    const r = stepFrame(frames, pb, false, 16, key);
    key = r.key;
    if (r.changed) notified++;
  }
  expect(notified).toBeGreaterThanOrEqual(1);
  expect(notified).toBeLessThan(5); // 30 quadros, bem menos que 30 avisos
  const steady = stepFrame(frames, pb, false, 16, key);
  expect(steady.changed).toBe(false);
});

test("pausar, trocar a velocidade e pular para o fim contam como mudança", () => {
  const pb = createPlayback(t.units);
  pb.play();
  let key = stepFrame(frames, pb, false, 16, "").key;
  pb.pause();
  let r = stepFrame(frames, pb, false, 16, key);
  expect(r.changed).toBe(true); key = r.key;
  pb.setSpeed(4);
  r = stepFrame(frames, pb, false, 16, key);
  expect(r.changed).toBe(true); key = r.key;
  pb.seekToEnd();
  r = stepFrame(frames, pb, false, 16, key);
  expect(r.changed).toBe(true);
  expect(r.cur.unitIndex).toBe(t.units.length - 1);
});

test("o quadro devolvido tem placar do lance atual e posição interpolada; sem movimento, a do lance", () => {
  const pb = createPlayback(t.units);
  pb.play();
  for (let i = 0; i < 40; i++) stepFrame(frames, pb, false, 100, "");
  const moving = stepFrame(frames, pb, false, 0, "");
  const still = stepFrame(frames, pb, true, 0, "");
  expect(moving.frame.score).toEqual(moving.cur.score);
  expect(still.frame.ball).toEqual(still.cur.ball);
});
