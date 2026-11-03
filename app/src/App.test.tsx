import { render, screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { vi } from "vitest";
import App from "./App";
import * as api from "./api";
import type { Timeline } from "./types";

const timeline: Timeline = {
  schemaVersion: 1,
  source: { engine: "bygfoot", upstreamCommit: "a".repeat(40), seed: 431199, country: "faroe_islands", team: "V. Gotu", weeks: 2, patches: [] },
  match: { fixtureId: 28, home: "N. Runavík", away: "V. Gotu", final: [1, 2] },
  units: [
    { minute: 0, time: 0, possession: 1, area: 1, result: [0, 0], event: { type: 1, team: 1, commentary: "Começa a partida." } },
    { minute: 40, time: 0, possession: 1, area: 3, result: [1, 2], event: { type: 5, team: 1, commentary: "Gol de Stian!", player: { id: 61, name: "Stian Mørch" } } },
  ],
};

afterEach(() => vi.restoreAllMocks());

test("lance sem comentário (opcional no contrato) não quebra nem mostra 'undefined'", async () => {
  const semTexto: Timeline = {
    ...timeline,
    units: [timeline.units[0], { minute: 12, time: 0, possession: 0, area: 1, result: [0, 0], event: { type: 7, team: 0 } }, timeline.units[1]],
  };
  vi.spyOn(api, "simulateMatch").mockResolvedValue(semTexto);
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  expect(await screen.findByText("Gol de Stian!")).toBeInTheDocument();
  expect(document.body).not.toHaveTextContent("undefined");
  expect(screen.getAllByRole("listitem")).toHaveLength(3);
});

test("mostra o formulário e nenhum resultado no início", () => {
  render(<App />);
  expect(screen.getByRole("button", { name: /simular partida/i })).toBeEnabled();
  expect(screen.getByLabelText(/seed/i)).toHaveValue(431199);
  expect(screen.getByText(/nenhuma partida simulada/i)).toBeInTheDocument();
});

test("simula e mostra placar e lances", async () => {
  const spy = vi.spyOn(api, "simulateMatch").mockResolvedValue(timeline);
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  expect(await screen.findByText("N. Runavík")).toBeInTheDocument();
  expect(screen.getByTestId("score")).toHaveTextContent("1 x 2");
  expect(screen.getByText("Gol de Stian!")).toBeInTheDocument();
  expect(spy).toHaveBeenCalledWith({ seed: 431199, country: "faroe_islands", team: "V. Gotu", weeks: 2 });
});

test("desabilita o botão enquanto simula", async () => {
  let resolve!: (t: Timeline) => void;
  vi.spyOn(api, "simulateMatch").mockReturnValue(new Promise((r) => (resolve = r)));
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  expect(screen.getByRole("button", { name: /simulando/i })).toBeDisabled();
  resolve(timeline);
  await waitFor(() => expect(screen.getByTestId("score")).toBeInTheDocument());
});

test("mostra o erro do motor sem esconder o formulário", async () => {
  vi.spyOn(api, "simulateMatch").mockRejectedValue({ kind: "engine_failed", message: "o motor falhou (código 3): time não encontrado" });
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  expect(await screen.findByRole("alert")).toHaveTextContent(/time não encontrado/);
  expect(screen.getByRole("button", { name: /simular partida/i })).toBeEnabled();
});

test("recusa seed inválida sem chamar o motor", async () => {
  const spy = vi.spyOn(api, "simulateMatch");
  render(<App />);
  const seed = screen.getByLabelText(/seed/i);
  await userEvent.clear(seed);
  await userEvent.type(seed, "-5");
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  expect(screen.getByRole("alert")).toHaveTextContent(/seed/i);
  expect(spy).not.toHaveBeenCalled();
});
