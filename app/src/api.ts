import { invoke } from "@tauri-apps/api/core";
import type { Catalog, CrestInfo, SimulateRequest, Timeline, UiError } from "./types";

export async function simulateMatch(req: SimulateRequest): Promise<Timeline> {
  return invoke<Timeline>("simulate_match", { ...req });
}

/** Continentes, países, ligas, copas e clubes da base do Bygfoot. */
export async function catalog(): Promise<Catalog> {
  return invoke<Catalog>("catalog");
}

/** Pasta onde o usuário coloca os escudos (criada se não existir). */
export async function crestFolder(): Promise<string> {
  return invoke<string>("crest_folder");
}

/** Situação do escudo de cada clube (encontrado, faltando ou inválido). */
export async function crestStatus(teams: string[]): Promise<CrestInfo[]> {
  return invoke<CrestInfo[]>("crest_status", { teams });
}

export function toUiError(e: unknown): UiError {
  if (e && typeof e === "object" && "message" in e && "kind" in e) return e as UiError;
  return { kind: "unknown", message: typeof e === "string" ? e : "Erro desconhecido" };
}
