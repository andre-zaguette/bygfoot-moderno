import type { Unit } from "../types";

export type Speed = 1 | 2 | 4;

export interface PlaybackState {
  index: number;
  /** 0..1 entre o lance atual e o próximo. */
  progress: number;
  /** Minuto de jogo exibido; nunca diminui. */
  minute: number;
  playing: boolean;
  speed: Speed;
  finished: boolean;
  elapsedMs: number;
}

export interface Playback {
  tick(dtMs: number): void;
  play(): void;
  pause(): void;
  setSpeed(s: Speed): void;
  seekToEnd(): void;
  restart(): void;
  state(): PlaybackState;
}

/** Maior avanço aceito por tick: aba em segundo plano não pode causar salto enorme. */
const MAX_TICK_MS = 250;
/** Duração fixa de cada lance da disputa de pênaltis (sem minuto próprio). */
const PENALTY_MS = 1500;
/** Intervalo mínimo entre dois lances (em minutos de jogo) para a animação ter tempo de acontecer. */
const MIN_GAP_MIN = 0.25;

export function createPlayback(units: Unit[], opts: { secondsPerMinute?: number } = {}): Playback {
  if (units.length === 0) throw new Error("timeline sem lances");
  const spm = (opts.secondsPerMinute ?? 4) * 1000;

  // minuto efetivo: -1 herda o anterior; nunca regride
  const minutes: number[] = [];
  let last = 0;
  for (const u of units) {
    last = u.minute >= 0 && u.time !== 3 ? Math.max(u.minute, last) : last;
    minutes.push(last);
  }
  // instante (ms a 1x) em que cada lance começa
  const start: number[] = [0];
  for (let i = 1; i < units.length; i++) {
    const gap = units[i].time === 3
      ? PENALTY_MS
      : Math.max(minutes[i] - minutes[i - 1], MIN_GAP_MIN) * spm;
    start.push(start[i - 1] + gap);
  }
  const end = start[start.length - 1];

  let elapsed = 0;
  let playing = false;
  let speed: Speed = 1;

  function indexAt(t: number): number {
    let lo = 0, hi = start.length - 1;
    while (lo < hi) {
      const mid = (lo + hi + 1) >> 1;
      if (start[mid] <= t) lo = mid; else hi = mid - 1;
    }
    return lo;
  }

  return {
    tick(dtMs) {
      if (!playing || elapsed >= end) return;
      elapsed = Math.min(end, elapsed + Math.min(Math.max(dtMs, 0), MAX_TICK_MS) * speed);
      if (elapsed >= end) playing = false;
    },
    play() { if (elapsed < end) playing = true; },
    pause() { playing = false; },
    setSpeed(s) { speed = s; },
    seekToEnd() { elapsed = end; playing = false; },
    restart() { elapsed = 0; playing = false; },
    state() {
      const index = indexAt(elapsed);
      const finished = elapsed >= end;
      const span = index < start.length - 1 ? start[index + 1] - start[index] : 0;
      return {
        index,
        progress: span > 0 ? (elapsed - start[index]) / span : 0,
        minute: minutes[index],
        playing,
        speed,
        finished,
        elapsedMs: elapsed,
      };
    },
  };
}
