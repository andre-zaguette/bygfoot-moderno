// Espelha schemas/match-timeline-v1.schema.json (o motor é a única fonte de verdade).
export interface Player { id: number; name: string }
export interface MatchEvent {
  type: number; team: number; commentary: string; player?: Player; player2?: Player;
}
export interface Unit {
  minute: number; time: number; possession: number; area: number;
  result: [number, number]; event: MatchEvent;
}
export interface Timeline {
  schemaVersion: 1;
  source: { engine: string; upstreamCommit: string; seed: number; country: string; team: string; weeks: number; patches: string[] };
  match: { fixtureId: number; home: string; away: string; final: [number, number] };
  units: Unit[];
}
export interface SimulateRequest { seed: number; country: string; team: string; weeks: number }
export interface UiError { kind: string; message: string }
