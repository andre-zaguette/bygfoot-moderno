import { render, screen, waitFor, within } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { vi } from "vitest";
import CrestGuide from "./CrestGuide";
import * as api from "../api";
import type { CrestInfo } from "../types";

const row = (team: string, slug: string, status: CrestInfo["status"], reason: string | null = null): CrestInfo => ({
  team, slug, file: `${slug}.png`, status, reason, data_url: status === "found" ? "data:image/png;base64,AAAA" : null,
});

beforeEach(() => {
  vi.spyOn(api, "crestFolder").mockResolvedValue("/dados/escudos");
  vi.spyOn(api, "crestStatus").mockResolvedValue([row("Santos", "santos", "found"), row("K. Klaksvík", "k-klaksvik", "missing"), row("Flamengo", "flamengo", "invalid", "o arquivo não é um PNG válido")]);
});
afterEach(() => vi.restoreAllMocks());

test("explica que o Bygfoot não traz escudos e que o usuário precisa adicioná-los", async () => {
  render(<CrestGuide teams={["Santos"]} onClose={() => {}} />);
  expect(screen.getByRole("heading", { name: /guia de escudos/i })).toBeInTheDocument();
  expect(screen.getByText(/não inclui escudos/i)).toBeInTheDocument();
  expect(screen.getByText(/você precisa adicionar/i)).toBeInTheDocument();
  expect(screen.getByText(/direito de usar/i)).toBeInTheDocument();
  await screen.findByText("/dados/escudos");
});

test("passo a passo numerado com formato, tamanho, nome do arquivo e pasta", async () => {
  render(<CrestGuide teams={["Santos"]} onClose={() => {}} />);
  const steps = within(screen.getByRole("list", { name: /passo a passo/i })).getAllByRole("listitem");
  expect(steps.length).toBeGreaterThanOrEqual(5);
  const text = steps.map((s) => s.textContent).join(" ");
  for (const must of ["PNG", "256", "512 KB", "k-klaksvik.png", "Verificar de novo", "minúsculas"]) expect(text).toContain(must);
  expect(await screen.findByText("/dados/escudos")).toBeInTheDocument();
});

test("lista cada clube da partida com o nome exato do arquivo e a situação", async () => {
  render(<CrestGuide teams={["Santos", "K. Klaksvík", "Flamengo"]} onClose={() => {}} />);
  const table = await screen.findByRole("table", { name: /escudos dos clubes da partida/i });
  const rows = within(table).getAllByRole("row").slice(1);
  expect(rows).toHaveLength(3);
  expect(rows[0]).toHaveTextContent(/Santos.*santos\.png.*Encontrado/);
  expect(rows[1]).toHaveTextContent(/K\. Klaksvík.*k-klaksvik\.png.*Faltando/);
  expect(rows[2]).toHaveTextContent(/Flamengo.*flamengo\.png.*Inválido.*não é um PNG válido/);
  expect(api.crestStatus).toHaveBeenCalledWith(["Santos", "K. Klaksvík", "Flamengo"]);
});

test("sem partida ainda, pede para simular e não consulta clubes", async () => {
  render(<CrestGuide teams={[]} onClose={() => {}} />);
  expect(await screen.findByText(/simule uma partida/i)).toBeInTheDocument();
  expect(api.crestStatus).not.toHaveBeenCalled();
});

test("Verificar de novo consulta outra vez e atualiza a situação", async () => {
  const s = vi.spyOn(api, "crestStatus");
  s.mockResolvedValueOnce([row("Santos", "santos", "missing")]).mockResolvedValueOnce([row("Santos", "santos", "found")]);
  render(<CrestGuide teams={["Santos"]} onClose={() => {}} />);
  expect(await screen.findByText("Faltando")).toBeInTheDocument();
  await userEvent.click(screen.getByRole("button", { name: /verificar de novo/i }));
  expect(await screen.findByText("Encontrado")).toBeInTheDocument();
  expect(s).toHaveBeenCalledTimes(2);
});

test("Copiar caminho coloca a pasta na área de transferência e avisa", async () => {
  const write = vi.fn().mockResolvedValue(undefined);
  Object.defineProperty(navigator, "clipboard", { value: { writeText: write }, configurable: true });
  render(<CrestGuide teams={[]} onClose={() => {}} />);
  await screen.findByText("/dados/escudos");
  await userEvent.click(screen.getByRole("button", { name: /copiar caminho/i }));
  expect(write).toHaveBeenCalledWith("/dados/escudos");
  expect(await screen.findByText(/caminho copiado/i)).toBeInTheDocument();
});

test("falha ao descobrir a pasta mostra erro claro e não quebra o guia", async () => {
  vi.spyOn(api, "crestFolder").mockRejectedValue({ kind: "crest_dir", message: "sem permissão" });
  render(<CrestGuide teams={[]} onClose={() => {}} />);
  expect(await screen.findByRole("alert")).toHaveTextContent(/sem permissão/);
  expect(screen.getByRole("heading", { name: /guia de escudos/i })).toBeInTheDocument();
});

test("falha ao consultar os clubes mostra erro e permite tentar de novo", async () => {
  vi.spyOn(api, "crestStatus").mockRejectedValueOnce({ kind: "invalid_params", message: "muitos clubes" }).mockResolvedValueOnce([row("Santos", "santos", "found")]);
  render(<CrestGuide teams={["Santos"]} onClose={() => {}} />);
  expect(await screen.findByRole("alert")).toHaveTextContent(/muitos clubes/);
  await userEvent.click(screen.getByRole("button", { name: /verificar de novo/i }));
  await waitFor(() => expect(screen.queryByRole("alert")).not.toBeInTheDocument());
  expect(await screen.findByText("Encontrado")).toBeInTheDocument();
});

test("foco vai para o título ao abrir e Voltar chama onClose", async () => {
  const onClose = vi.fn();
  render(<CrestGuide teams={[]} onClose={onClose} />);
  expect(screen.getByRole("heading", { name: /guia de escudos/i })).toHaveFocus();
  await userEvent.click(screen.getByRole("button", { name: /voltar/i }));
  expect(onClose).toHaveBeenCalled();
});

test("Escape fecha o guia", async () => {
  const onClose = vi.fn();
  render(<CrestGuide teams={[]} onClose={onClose} />);
  await userEvent.keyboard("{Escape}");
  expect(onClose).toHaveBeenCalled();
});
