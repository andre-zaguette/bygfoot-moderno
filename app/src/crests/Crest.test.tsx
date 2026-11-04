import { render, screen } from "@testing-library/react";
import Crest, { initials, hueOf } from "./Crest";
import type { CrestInfo } from "../types";

const found: CrestInfo = { team: "Santos", slug: "santos", file: "santos.png", status: "found", reason: null, data_url: "data:image/png;base64,AAAA" };

test("com escudo encontrado mostra a imagem do usuário", () => {
  render(<Crest team="Santos" info={found} />);
  const img = screen.getByRole("img", { name: "Escudo de Santos" });
  expect(img).toHaveAttribute("src", "data:image/png;base64,AAAA");
});

test.each([undefined, { ...found, status: "missing", data_url: null }, { ...found, status: "invalid", data_url: null, reason: "x" }])(
  "sem escudo utilizável mostra o emblema gerado com as iniciais (%#)",
  (info) => {
    render(<Crest team="K. Klaksvík" info={info as CrestInfo | undefined} />);
    const el = screen.getByRole("img", { name: "Emblema de K. Klaksvík (sem escudo)" });
    expect(el).toHaveTextContent("KK");
    expect(el.tagName.toLowerCase()).toBe("svg");
  },
);

test("iniciais: duas primeiras palavras ou duas primeiras letras", () => {
  expect(initials("K. Klaksvík")).toBe("KK");
  expect(initials("Real Madrid CF")).toBe("RM");
  expect(initials("Santos")).toBe("SA");
  expect(initials("  ")).toBe("?");
  expect(initials("…")).toBe("?");
});

test("a cor do emblema é estável por nome e varia entre nomes", () => {
  expect(hueOf("Santos")).toBe(hueOf("Santos"));
  expect(hueOf("Santos")).not.toBe(hueOf("Flamengo"));
  expect(hueOf("Santos")).toBeGreaterThanOrEqual(0);
  expect(hueOf("Santos")).toBeLessThan(360);
});
