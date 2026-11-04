import { render, screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { vi } from "vitest";
vi.mock("./live/PitchView", () => ({ default: () => <div data-testid="pitch" /> }));
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

const crest = (team: string, status: "found" | "missing", slug: string) => ({
  team, slug, file: `${slug}.png`, status, reason: null, data_url: status === "found" ? "data:image/png;base64,AAAA" : null,
});
beforeEach(() => {
  vi.spyOn(api, "crestStatus").mockImplementation(async (teams) => teams.map((t) => crest(t, "missing", t.toLowerCase())));
  vi.spyOn(api, "crestFolder").mockResolvedValue("/dados/escudos");
});
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

test("depois de simular, dá para assistir a partida e o placar final é o do motor", async () => {
  vi.spyOn(api, "simulateMatch").mockResolvedValue(timeline);
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  await userEvent.click(await screen.findByRole("button", { name: /assistir partida/i }));
  expect(screen.getByRole("group", { name: /partida ao vivo/i })).toBeInTheDocument();
  expect(screen.getByTestId("pitch")).toBeInTheDocument();
  await userEvent.click(screen.getByRole("button", { name: /pular para o fim/i }));
  expect(screen.getAllByTestId("score").some((n) => n.textContent === "1 x 2")).toBe(true);
  await userEvent.click(screen.getByRole("button", { name: /ver só os lances/i }));
  expect(screen.queryByRole("group", { name: /partida ao vivo/i })).not.toBeInTheDocument();
});

test("uma nova simulação sai do modo de assistir", async () => {
  vi.spyOn(api, "simulateMatch").mockResolvedValue(timeline);
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  await userEvent.click(await screen.findByRole("button", { name: /assistir partida/i }));
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  await waitFor(() => expect(screen.queryByRole("group", { name: /partida ao vivo/i })).not.toBeInTheDocument());
});

test("o foco acompanha a troca de modo: abre na partida e volta para 'Assistir partida'", async () => {
  vi.spyOn(api, "simulateMatch").mockResolvedValue(timeline);
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  await userEvent.click(await screen.findByRole("button", { name: /assistir partida/i }));
  expect(screen.getByRole("group", { name: /partida ao vivo/i })).toHaveFocus();
  await userEvent.click(screen.getByRole("button", { name: /ver só os lances/i }));
  expect(screen.getByRole("button", { name: /assistir partida/i })).toHaveFocus();
});

test("depois de simular, consulta os escudos dos dois clubes e mostra o emblema no resultado", async () => {
  vi.spyOn(api, "simulateMatch").mockResolvedValue(timeline);
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  await screen.findByText("N. Runavík");
  await waitFor(() => expect(api.crestStatus).toHaveBeenCalledWith(["N. Runavík", "V. Gotu"]));
  expect(await screen.findByRole("img", { name: "Emblema de N. Runavík (sem escudo)" })).toBeInTheDocument();
});

test("escudo do usuário aparece no lugar do emblema", async () => {
  vi.spyOn(api, "simulateMatch").mockResolvedValue(timeline);
  vi.spyOn(api, "crestStatus").mockResolvedValue([crest("N. Runavík", "found", "n-runavik"), crest("V. Gotu", "missing", "v-gotu")]);
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  expect(await screen.findByRole("img", { name: "Escudo de N. Runavík" })).toBeInTheDocument();
  expect(screen.getByRole("img", { name: "Emblema de V. Gotu (sem escudo)" })).toBeInTheDocument();
});

test("falha ao consultar escudos não atrapalha o resultado", async () => {
  vi.spyOn(api, "simulateMatch").mockResolvedValue(timeline);
  vi.spyOn(api, "crestStatus").mockRejectedValue({ kind: "crest_dir", message: "sem pasta" });
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  expect(await screen.findByTestId("score")).toHaveTextContent("1 x 2");
  expect(screen.queryByRole("alert")).not.toBeInTheDocument();
});

test("o Guia de escudos abre, mostra a pasta e Voltar devolve o foco ao botão e mantém a partida", async () => {
  vi.spyOn(api, "simulateMatch").mockResolvedValue(timeline);
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /simular partida/i }));
  await screen.findByTestId("score");
  const open = screen.getByRole("button", { name: /guia de escudos/i });
  await userEvent.click(open);
  expect(screen.getByRole("heading", { name: /guia de escudos/i })).toHaveFocus();
  expect(await screen.findByText("/dados/escudos")).toBeInTheDocument();
  expect(await screen.findByRole("table", { name: /escudos dos clubes da partida/i })).toBeInTheDocument();
  await userEvent.click(screen.getByRole("button", { name: /^voltar$/i }));
  expect(screen.getByRole("button", { name: /guia de escudos/i })).toHaveFocus();
  expect(screen.getByTestId("score")).toHaveTextContent("1 x 2");
});

test("o guia abre mesmo antes de qualquer partida", async () => {
  render(<App />);
  await userEvent.click(screen.getByRole("button", { name: /guia de escudos/i }));
  expect(await screen.findByText(/simule uma partida/i)).toBeInTheDocument();
});
