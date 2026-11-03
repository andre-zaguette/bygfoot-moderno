import { act, render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { vi } from "vitest";
import type { Timeline } from "../types";

const captured = vi.hoisted(() => ({ props: null as any }));
vi.mock("./PitchView", () => ({
  default: (props: unknown) => { captured.props = props; return <div data-testid="pitch" />; },
}));
import LiveMatch from "./LiveMatch";

const u = (minute: number, type: number, result: [number, number], commentary?: string) => ({
  minute, time: 0, possession: 0, area: 1, result,
  event: { type, team: 0, ...(commentary ? { commentary } : {}) },
});
const timeline: Timeline = {
  schemaVersion: 1,
  source: { engine: "bygfoot", upstreamCommit: "a".repeat(40), seed: 5, country: "x", team: "Time Azul", weeks: 1, patches: [] },
  match: { fixtureId: 1, home: "Time Azul", away: "Time Vermelho", final: [1, 0] },
  units: [
    u(0, 1, [0, 0], "Começa a partida."),
    { ...u(20, 0, [0, 0], "Troca de passes."), event: { type: 0, team: 0, commentary: "Troca de passes.", player: { id: 1, name: "Ana" }, player2: { id: 2, name: "Bia" } } },
    u(40, 10, [1, 0], "Gol do Azul!"),
    u(60, 0, [1, 0]),
    u(90, 4, [1, 0], "Fim de jogo."),
  ],
};

/** Avança o relógio como o PitchView faria e avisa a interface. */
function advance(ms: number) {
  act(() => {
    const { playback, frames, onState } = captured.props;
    for (let t = 0; t < ms; t += 100) playback.tick(100);
    const st = playback.state();
    onState(st, frames[st.index]);
  });
}
const score = () => screen.getByTestId("score").textContent;

test("começa tocando em 0 x 0 com os nomes dos times e o rótulo de aproximação visual", () => {
  render(<LiveMatch timeline={timeline} />);
  expect(score()).toBe("0 x 0");
  expect(screen.getByText("Time Azul")).toBeInTheDocument();
  expect(screen.getByText("Time Vermelho")).toBeInTheDocument();
  expect(screen.getByText(/aproximação visual/i)).toBeInTheDocument();
  expect(captured.props.playback.state().playing).toBe(true);
});

test("o gol só aparece no placar quando o lance do gol é alcançado", () => {
  render(<LiveMatch timeline={timeline} />);
  advance(10_000); // 10 s ≈ minuto 2,5: ainda antes do gol (minuto 40)
  expect(score()).toBe("0 x 0");
  while (captured.props.playback.state().index < 1) advance(1000);
  expect(score()).toBe("0 x 0"); // lance 1 alcançado, gol ainda não
  while (captured.props.playback.state().index < 2) advance(1000);
  expect(score()).toBe("1 x 0");
});

test("Pular para o fim mostra o placar final do motor e o botão Rever", async () => {
  render(<LiveMatch timeline={timeline} />);
  await userEvent.click(screen.getByRole("button", { name: /pular para o fim/i }));
  expect(score()).toBe("1 x 0");
  expect(screen.getByRole("button", { name: /rever/i })).toBeInTheDocument();
});

test("Rever volta ao início, pausado em 0 x 0, e depois dá para tocar de novo", async () => {
  render(<LiveMatch timeline={timeline} />);
  await userEvent.click(screen.getByRole("button", { name: /pular para o fim/i }));
  await userEvent.click(screen.getByRole("button", { name: /rever/i }));
  expect(score()).toBe("0 x 0");
  expect(captured.props.playback.state().finished).toBe(false);
});

test("Espaço pausa e retoma; os botões 1x, 2x e 4x e as teclas 1, 2 e 4 trocam a velocidade", async () => {
  render(<LiveMatch timeline={timeline} />);
  const region = screen.getByRole("group", { name: /partida/i });
  region.focus();
  await userEvent.keyboard(" ");
  expect(captured.props.playback.state().playing).toBe(false);
  expect(screen.getByRole("button", { name: /retomar/i })).toBeInTheDocument();
  await userEvent.keyboard(" ");
  expect(captured.props.playback.state().playing).toBe(true);
  await userEvent.keyboard("4");
  expect(captured.props.playback.state().speed).toBe(4);
  expect(screen.getByRole("button", { name: "4x" })).toHaveAttribute("aria-pressed", "true");
  await userEvent.click(screen.getByRole("button", { name: "2x" }));
  expect(captured.props.playback.state().speed).toBe(2);
  await userEvent.keyboard("1");
  expect(captured.props.playback.state().speed).toBe(1);
});

test("Espaço com o foco num botão aciona só o botão (não alterna duas vezes)", async () => {
  render(<LiveMatch timeline={timeline} />);
  const pause = screen.getByRole("button", { name: /pausar/i });
  pause.focus();
  await userEvent.keyboard(" ");
  expect(captured.props.playback.state().playing).toBe(false);
});

test("lance sem comentário não mostra 'undefined' e usa legenda neutra", () => {
  render(<LiveMatch timeline={timeline} />);
  captured.props.playback.setSpeed(4);
  while (captured.props.playback.state().index < 3) advance(1000); // lance do minuto 60, sem comentário
  expect(screen.getByTestId("caption").textContent).toBeTruthy();
  expect(document.body).not.toHaveTextContent("undefined");
});

test("os controles são botões acessíveis por teclado, com nomes em português", () => {
  render(<LiveMatch timeline={timeline} />);
  for (const name of [/pausar/i, "1x", "2x", "4x", /pular para o fim/i]) {
    expect(screen.getByRole("button", { name })).toBeEnabled();
  }
});

test("lances importantes são anunciados numa região aria-live; lances comuns não", () => {
  render(<LiveMatch timeline={timeline} />);
  const live = screen.getByTestId("announcer");
  expect(live).toHaveAttribute("aria-live", "polite");
  while (captured.props.playback.state().index < 1) advance(1000); // lance comum
  expect(live.textContent).not.toMatch(/Troca de passes/);
  while (captured.props.playback.state().index < 2) advance(1000); // chega ao gol
  expect(live.textContent).toMatch(/Gol do Azul/);
});

describe("camada de análise", () => {
  const toggle = (name: RegExp) => screen.getByRole("button", { name });

  test("começa desligada: sem calor no campo, sem painel de passes e sem legenda de nomes", () => {
    render(<LiveMatch timeline={timeline} />);
    for (const n of [/calor/i, /passes/i, /nomes/i]) expect(toggle(n)).toHaveAttribute("aria-pressed", "false");
    expect(captured.props.heat).toBeNull();
    expect(screen.queryByRole("region", { name: /passes mais frequentes/i })).not.toBeInTheDocument();
    expect(screen.queryByTestId("names")).not.toBeInTheDocument();
  });

  test("Calor liga e desliga o calor enviado ao campo, rotulado como aproximação", async () => {
    render(<LiveMatch timeline={timeline} />);
    await userEvent.click(toggle(/calor/i));
    expect(toggle(/calor/i)).toHaveAttribute("aria-pressed", "true");
    expect(captured.props.heat).toHaveLength(2);
    await userEvent.click(toggle(/calor/i));
    expect(captured.props.heat).toBeNull();
  });

  test("Passes lista os pares citados só depois de acontecerem", async () => {
    render(<LiveMatch timeline={timeline} />);
    await userEvent.click(toggle(/passes/i));
    const panel = screen.getByRole("region", { name: /passes mais frequentes/i });
    expect(panel).toHaveTextContent(/nenhum passe/i);
    while (captured.props.playback.state().index < 1) advance(1000);
    expect(panel).toHaveTextContent("Ana");
    expect(panel).toHaveTextContent("Bia");
  });

  test("Nomes mostra os jogadores do lance atual", async () => {
    render(<LiveMatch timeline={timeline} />);
    await userEvent.click(toggle(/nomes/i));
    while (captured.props.playback.state().index < 1) advance(1000);
    expect(screen.getByTestId("names")).toHaveTextContent("Ana");
    expect(screen.getByTestId("names")).toHaveTextContent("Bia");
  });

  test("com qualquer camada ligada aparece a posse em %", async () => {
    render(<LiveMatch timeline={timeline} />);
    expect(screen.queryByTestId("possession")).not.toBeInTheDocument();
    await userEvent.click(toggle(/calor/i));
    expect(screen.getByTestId("possession")).toHaveTextContent(/%/);
  });
});
