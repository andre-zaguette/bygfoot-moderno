import { render, screen, within } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { vi } from "vitest";
import TeamPicker, { normalize, suggestedWeeks, firstSelection } from "./TeamPicker";
import type { Catalog } from "../types";

const catalog: Catalog = {
  continents: [
    { id: "europe", countries: [
      { sid: "faroe_islands", name: "Faroe Islands", flag: "flag_faroe_islands.png", cups: [{ sid: "fo_cup", name: "Faroe Cup" }], leagues: [
        { sid: "faroe_islands1", name: "Betri Deildin", layer: 1, firstWeek: 2, teams: ["V. Gotu", "K. Klaksvík", "HB Tórshavn"] },
        { sid: "faroe_islands2", name: "1. Deild", layer: 2, firstWeek: 2, teams: ["Skála", "Víkingur"] },
      ] },
    ] },
    { id: "south_america", countries: [
      { sid: "brazil", name: "Brazil", flag: null, cups: [], leagues: [{ sid: "brazil1", name: "Serie A", layer: 1, firstWeek: 7, teams: ["Santos", "Flamengo"] }] },
    ] },
    { id: "miscellaneous", countries: [{ sid: "wc", name: "World Cup", flag: null, cups: [], leagues: [] }] },
  ],
};
const value = { country: "faroe_islands", league: "faroe_islands1", team: "V. Gotu" };

test("normalize ignora acentos e caixa", () => {
  expect(normalize("K. Klaksvík")).toBe(normalize("k. klaksvik"));
  expect(normalize("São Paulo")).toBe("sao paulo");
});

test("semanas sugeridas cobrem a primeira rodada da liga", () => {
  expect(suggestedWeeks(7)).toBe(8); // a rodada 1 pode ser folga do clube: uma semana de margem
  expect(suggestedWeeks(5)).toBe(6); // sweden5j falhou com 5 e passou com 6
  expect(suggestedWeeks(1)).toBe(2);
  expect(suggestedWeeks(null)).toBe(12); // ligas sem first_week (p.ex. peru3) só jogam depois da 8ª semana
});

test("firstSelection escolhe a primeira liga com clubes e o primeiro clube", () => {
  expect(firstSelection(catalog, "brazil")).toEqual({ country: "brazil", league: "brazil1", team: "Santos", firstWeek: 7 });
  expect(firstSelection(catalog, "wc")).toEqual({ country: "wc", league: "", team: "", firstWeek: null });
  expect(firstSelection(catalog, "nao-existe")).toBeNull();
});

test("países agrupados por continente em português e valor atual selecionado", () => {
  render(<TeamPicker catalog={catalog} value={value} onSelect={() => {}} />);
  const country = screen.getByRole("combobox", { name: "País" });
  expect(country).toHaveValue("faroe_islands");
  expect(within(country).getByRole("group", { name: "Europa" })).toBeInTheDocument();
  expect(within(country).getByRole("group", { name: "América do Sul" })).toBeInTheDocument();
  expect(within(country).getByRole("group", { name: "Seleções e competições" })).toBeInTheDocument();
  expect(screen.getByRole("combobox", { name: "Liga" })).toHaveValue("faroe_islands1");
  expect(screen.getByRole("option", { name: "Betri Deildin (3 clubes)" })).toBeInTheDocument();
  expect(screen.getByRole("listbox", { name: "Clube" })).toHaveValue("V. Gotu");
});

test("trocar o país escolhe a primeira liga e o primeiro clube e sugere as semanas", async () => {
  const onSelect = vi.fn();
  render(<TeamPicker catalog={catalog} value={value} onSelect={onSelect} />);
  await userEvent.selectOptions(screen.getByRole("combobox", { name: "País" }), "brazil");
  expect(onSelect).toHaveBeenCalledWith({ country: "brazil", league: "brazil1", team: "Santos", firstWeek: 7 });
});

test("trocar a liga escolhe o primeiro clube dela", async () => {
  const onSelect = vi.fn();
  render(<TeamPicker catalog={catalog} value={value} onSelect={onSelect} />);
  await userEvent.selectOptions(screen.getByRole("combobox", { name: "Liga" }), "faroe_islands2");
  expect(onSelect).toHaveBeenCalledWith({ country: "faroe_islands", league: "faroe_islands2", team: "Skála", firstWeek: 2 });
});

test("escolher um clube da lista chama onSelect com ele", async () => {
  const onSelect = vi.fn();
  render(<TeamPicker catalog={catalog} value={value} onSelect={onSelect} />);
  await userEvent.selectOptions(screen.getByRole("listbox", { name: "Clube" }), "K. Klaksvík");
  expect(onSelect).toHaveBeenCalledWith({ country: "faroe_islands", league: "faroe_islands1", team: "K. Klaksvík", firstWeek: 2 });
});

test("o filtro acha clubes sem depender de acentos e avisa quando não há resultado", async () => {
  render(<TeamPicker catalog={catalog} value={value} onSelect={() => {}} />);
  const box = screen.getByRole("searchbox", { name: /filtrar clubes/i });
  await userEvent.type(box, "klaksvik");
  expect(within(screen.getByRole("listbox", { name: "Clube" })).getAllByRole("option")).toHaveLength(1);
  await userEvent.clear(box);
  await userEvent.type(box, "zzz");
  expect(screen.getByRole("status")).toHaveTextContent(/nenhum clube/i);
});

test("país sem ligas mostra aviso e não oferece clubes", () => {
  render(<TeamPicker catalog={catalog} value={{ country: "wc", league: "", team: "" }} onSelect={() => {}} />);
  expect(screen.getByText(/não tem ligas com clubes/i)).toBeInTheDocument();
  expect(screen.queryByRole("listbox", { name: "Clube" })).not.toBeInTheDocument();
});

test("lista as copas do país selecionado", () => {
  render(<TeamPicker catalog={catalog} value={value} onSelect={() => {}} />);
  expect(screen.getByText(/Faroe Cup/)).toBeInTheDocument();
});
