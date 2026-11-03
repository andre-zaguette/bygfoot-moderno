// Camada de análise (visual C). Só agrega o que o motor informou nos lances já ocorridos:
// área do campo, posse e jogadores citados. NÃO há posição individual de jogador no motor,
// então nada aqui associa um nome a um ponto do campo. É uma aproximação visual.
import type { Player, Timeline } from "../types";

export interface PassLink { team: 0 | 1; a: Player; b: Player; weight: number }
export interface Overlay {
  /** heat[time][área]: lances com a posse do time naquela área (0 defesa, 1 meio, 2 ataque, do ponto de vista do time). */
  heat: number[][];
  /** Pares de jogadores citados juntos em lances de jogo corrido, do mais frequente ao menos. */
  links: PassLink[];
  /** Posse em % (casa, fora), soma 100. */
  possession: [number, number];
}

export function buildOverlay(t: Timeline, upToIndex: number): Overlay {
  const last = Math.min(Math.max(upToIndex, 0), t.units.length - 1);
  const heat = [[0, 0, 0], [0, 0, 0]];
  const pairs = new Map<string, PassLink>();
  const poss = [0, 0];

  for (let i = 0; i <= last; i++) {
    const u = t.units[i];
    const team = (u.possession === 1 ? 1 : 0) as 0 | 1;
    heat[team][Math.min(Math.max(u.area, 0), 2)]++;
    poss[team]++;
    const { player, player2 } = u.event;
    // Só jogo corrido (tipo 0). Em gol, falta, cartão e substituição o 2º jogador é o goleiro, o
    // faltoso, a vítima ou quem sai: não é companheiro de time, então não conta como combinação.
    if (u.event.type === 0 && player && player2) {
      const [a, b] = player.id <= player2.id ? [player, player2] : [player2, player];
      const key = `${team}:${a.id}:${b.id}`;
      const cur = pairs.get(key);
      if (cur) cur.weight++;
      else pairs.set(key, { team, a, b, weight: 1 });
    }
  }

  const n = poss[0] + poss[1];
  const home = Math.round((poss[0] / n) * 100);
  const links = [...pairs.values()].sort(
    // desempate por id (nunca por nome): a ordem de nomes muda com o idioma do sistema
    (x, y) => y.weight - x.weight || x.a.id - y.a.id || x.b.id - y.b.id || x.team - y.team,
  );
  return { heat, links, possession: [home, 100 - home] };
}
