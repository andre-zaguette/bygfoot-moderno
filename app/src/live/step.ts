import type { Frame } from "./choreography";
import { lerpFrame } from "./interpolate";
import type { Playback, PlaybackState } from "./playback";

const smooth = (t: number) => t * t * (3 - 2 * t);

export interface StepResult {
  st: PlaybackState;
  /** Lance atual (placar, legenda e efeito vêm dele). */
  cur: Frame;
  /** Quadro a desenhar: posições interpoladas entre o lance atual e o próximo. */
  frame: Frame;
  key: string;
  /** A interface (HUD, anúncios) só precisa ser avisada quando isto for true. */
  changed: boolean;
}

/** Um quadro de animação: avança o relógio, escolhe o quadro e diz se a interface precisa saber. */
export function stepFrame(frames: Frame[], pb: Playback, reducedMotion: boolean, dtMs: number, lastKey: string): StepResult {
  pb.tick(dtMs);
  const st = pb.state();
  const cur = frames[st.index];
  const next = frames[Math.min(st.index + 1, frames.length - 1)];
  const frame = lerpFrame(cur, next, smooth(st.progress), reducedMotion);
  const key = `${st.index}|${st.playing}|${st.speed}|${st.finished}`;
  return { st, cur, frame, key, changed: key !== lastKey };
}
