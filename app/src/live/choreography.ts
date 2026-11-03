// Coreografia: converte cada lance do motor em posições de bola e jogadores.
// É uma APROXIMAÇÃO VISUAL (o motor não fornece coordenadas). Função pura e determinística:
// nada aqui calcula placar nem decide autor de gol; o placar de cada quadro é copiado do lance.
import type { Timeline, Unit } from "../types";

export const FIELD_W = 105;
export const FIELD_H = 68;

export interface Vec { x: number; y: number }
export type EffectKind =
  | "start" | "halftime" | "extratime" | "end" | "chance" | "penalty" | "freekick" | "goal"
  | "post" | "miss" | "save" | "corner" | "foul" | "yellow" | "red" | "injury"
  | "substitution" | "style" | "shootout";
export interface Effect { kind: EffectKind; team: -1 | 0 | 1 }
export interface Frame {
  unitIndex: number;
  minute: number;
  ball: Vec;
  players: { home: Vec[]; away: Vec[] };
  effect: Effect | null;
  score: [number, number];
  caption: string;
}

const KIND: Record<number, EffectKind> = {
  1: "start", 2: "halftime", 3: "extratime", 4: "end", 6: "chance", 7: "chance", 8: "penalty",
  9: "freekick", 10: "goal", 11: "goal", 12: "post", 13: "miss", 14: "save", 15: "save",
  16: "post", 17: "save", 18: "corner", 19: "foul", 20: "yellow", 21: "red", 22: "red",
  23: "red", 24: "injury", 25: "injury", 26: "shootout", 31: "substitution",
  32: "style", 33: "style", 34: "style", 35: "style", 36: "style", 37: "style",
  38: "style", 39: "style", 40: "style",
};

const FALLBACK: Record<EffectKind, string> = {
  start: "Começa a partida.", halftime: "Fim do primeiro tempo.", extratime: "Começa a prorrogação.",
  end: "Fim de jogo.", chance: "Chance de gol.", penalty: "Pênalti.", freekick: "Falta perigosa.",
  goal: "Gol!", post: "Na trave.", miss: "Chute para fora.", save: "Grande defesa.",
  corner: "Escanteio.", foul: "Falta.", yellow: "Cartão amarelo.", red: "Cartão vermelho.",
  injury: "Jogador machucado.", substitution: "Substituição.", style: "Mudança de postura da equipe.",
  shootout: "Disputa de pênaltis.",
};

// 4-4-2 do time da casa (ataca para a direita), x/y em metros
const BASE: Vec[] = [
  [5, 34], [22, 10], [20, 26], [20, 42], [22, 58], [40, 10], [38, 26], [38, 42], [40, 58], [58, 24], [58, 44],
].map(([x, y]) => ({ x: x * 0.92 + 2, y }));

function mulberry32(seed: number): () => number {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

const clamp = (v: number, lo: number, hi: number) => Math.min(hi, Math.max(lo, v));
const AREA_X = [18, 52, 86];

export function buildChoreography(t: Timeline): Frame[] {
  const frames: Frame[] = [];
  let prev: [number, number] = [0, 0];
  t.units.forEach((u: Unit, i) => {
    const rnd = mulberry32((t.source.seed ^ Math.imul(i + 1, 2654435761)) >>> 0);
    const kind = KIND[u.event.type];
    const attHome = u.possession === 0;
    const areaX = AREA_X[clamp(u.area, 0, 2)];
    const baseX = attHome ? areaX : FIELD_W - areaX;
    const jitterY = 8 + rnd() * (FIELD_H - 16);
    const goalX = attHome ? FIELD_W - 8.5 : 8.5; // perto da baliza que o time com a bola ataca

    let ball: Vec = { x: baseX, y: jitterY };
    let effect: Effect | null = null;
    const evTeam = (u.event.team === 1 ? 1 : u.event.team === 0 ? 0 : -1) as -1 | 0 | 1;
    if (kind) effect = { kind, team: evTeam };

    switch (kind) {
      case "start": case "halftime": case "extratime": case "end": case "shootout":
        ball = { x: FIELD_W / 2, y: FIELD_H / 2 }; break;
      case "goal": {
        const homeScored = u.result[0] > prev[0];
        const awayScored = u.result[1] > prev[1];
        // lado lido do placar do motor; sem mudança de placar (dado inconsistente), usa a posse
        const toRight = homeScored || (!awayScored && attHome);
        ball = { x: toRight ? FIELD_W - 0.4 : 0.4, y: FIELD_H / 2 + (rnd() - 0.5) * 6 };
        effect = { kind: "goal", team: toRight ? 0 : 1 };
        break;
      }
      case "chance": case "post": case "miss": case "save":
        ball = { x: goalX, y: FIELD_H / 2 + (rnd() - 0.5) * 12 }; break;
      case "penalty":
        ball = { x: attHome ? FIELD_W - 11 : 11, y: FIELD_H / 2 }; break;
      case "corner":
        ball = { x: attHome ? FIELD_W - 0.5 : 0.5, y: rnd() < 0.5 ? 0.5 : FIELD_H - 0.5 }; break;
      default: break;
    }

    const neutral = kind === "start" || kind === "halftime" || kind === "extratime" || kind === "end" || kind === "shootout";
    const place = (base: Vec[], homeTeam: boolean): Vec[] => {
      const attacking = homeTeam === attHome;
      const k = neutral ? 0 : attacking ? 0.45 : 0.25;
      const dx = (ball.x - FIELD_W / 2) * k;
      const dy = neutral ? 0 : (ball.y - FIELD_H / 2) * 0.15;
      return base.map((b, n) => {
        const bx = homeTeam ? b.x : FIELD_W - b.x;
        const by = homeTeam ? b.y : FIELD_H - b.y;
        const j = neutral ? 0 : 1.2;
        const shift = n === 0 ? 0.1 : 1; // o goleiro quase não sai da área
        return {
          x: clamp(bx + dx * shift + (rnd() - 0.5) * 2 * j, 0.5, FIELD_W - 0.5),
          y: clamp(by + dy * shift + (rnd() - 0.5) * 2 * j, 0.5, FIELD_H - 0.5),
        };
      });
    };

    frames.push({
      unitIndex: i,
      minute: u.minute,
      ball: { x: clamp(ball.x, 0, FIELD_W), y: clamp(ball.y, 0, FIELD_H) },
      players: { home: place(BASE, true), away: place(BASE, false) },
      effect,
      score: [u.result[0], u.result[1]],
      caption: u.event.commentary?.trim() || (kind ? FALLBACK[kind] : "O jogo segue."),
    });
    prev = u.result;
  });
  return frames;
}
