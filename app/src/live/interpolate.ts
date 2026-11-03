import type { Frame, Vec } from "./choreography";

const lerp = (a: number, b: number, t: number) => a + (b - a) * t;
const lerpVec = (a: Vec, b: Vec, t: number): Vec => ({ x: lerp(a.x, b.x, t), y: lerp(a.y, b.y, t) });

/**
 * Posições entre o lance atual (a) e o próximo (b). Placar, legenda, efeito e minuto
 * são sempre os do lance atual: o visual nunca antecipa o que o motor ainda não "disse".
 * Com movimento reduzido, não há posições intermediárias: devolve o quadro atual.
 */
export function lerpFrame(a: Frame, b: Frame, t: number, reducedMotion = false): Frame {
  if (reducedMotion) return a;
  const k = Math.min(1, Math.max(0, t));
  return {
    ...a,
    ball: lerpVec(a.ball, b.ball, k),
    players: {
      home: a.players.home.map((p, i) => lerpVec(p, b.players.home[i], k)),
      away: a.players.away.map((p, i) => lerpVec(p, b.players.away[i], k)),
    },
  };
}
