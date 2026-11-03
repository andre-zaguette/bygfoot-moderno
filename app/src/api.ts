import { invoke } from "@tauri-apps/api/core";
import type { SimulateRequest, Timeline, UiError } from "./types";

export async function simulateMatch(req: SimulateRequest): Promise<Timeline> {
  return invoke<Timeline>("simulate_match", { ...req });
}

export function toUiError(e: unknown): UiError {
  if (e && typeof e === "object" && "message" in e && "kind" in e) return e as UiError;
  return { kind: "unknown", message: typeof e === "string" ? e : "Erro desconhecido" };
}
