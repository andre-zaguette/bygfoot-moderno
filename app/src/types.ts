// Espelha schemas/match-timeline-v1.schema.json (o motor é a única fonte de verdade).
export interface Player { id: number; name: string }
export interface MatchEvent {
  type: number; team: number; commentary?: string; player?: Player; player2?: Player;
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

/** Situação do escudo de um clube na pasta do usuário (espelha `CrestInfo` do Rust). */
export interface CrestInfo {
  team: string; slug: string; file: string;
  status: "found" | "missing" | "invalid";
  reason: string | null; data_url: string | null;
}

/** Base de dados do Bygfoot (espelha `catalog.rs`; campos em camelCase). */
export interface CatalogLeague { sid: string; name: string; layer: number | null; firstWeek: number | null; teams: string[] }
export interface CatalogCup { sid: string; name: string }
export interface CatalogCountry { sid: string; name: string; flag: string | null; leagues: CatalogLeague[]; cups: CatalogCup[] }
export interface CatalogContinent { id: string; countries: CatalogCountry[] }
export interface Catalog { continents: CatalogContinent[] }
