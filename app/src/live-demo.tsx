// Vitrine SÓ PARA DESENVOLVIMENTO: toca uma partida de fixture sem o backend Tauri.
// Abra http://localhost:1420/live-demo.html com `npm run dev`. Não entra no build do app.
import ReactDOM from "react-dom/client";
import LiveMatch from "./live/LiveMatch";
import type { Timeline } from "./types";
import m1 from "./live/__fixtures__/match-seed1.json";
import m7 from "./live/__fixtures__/match-seed7.json";
import "./styles.css";

const fixtures: Record<string, Timeline> = { "1": m1 as unknown as Timeline, "7": m7 as unknown as Timeline };
const id = new URLSearchParams(location.search).get("seed") ?? "1";

ReactDOM.createRoot(document.getElementById("root")!).render(
  <main className="app"><LiveMatch timeline={fixtures[id] ?? fixtures["1"]} /></main>,
);
