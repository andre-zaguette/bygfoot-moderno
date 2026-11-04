import { useCallback, useEffect, useRef, useState } from "react";
import * as api from "../api";
import type { CrestInfo } from "../types";
import Crest from "./Crest";

const STATUS_TEXT: Record<CrestInfo["status"], string> = { found: "Encontrado", missing: "Faltando", invalid: "Inválido" };

export default function CrestGuide({ teams, onClose }: { teams: string[]; onClose: () => void }) {
  const title = useRef<HTMLHeadingElement>(null);
  const [folder, setFolder] = useState<string | null>(null);
  const [folderErr, setFolderErr] = useState<string | null>(null);
  const [rows, setRows] = useState<CrestInfo[]>([]);
  const [rowsErr, setRowsErr] = useState<string | null>(null);
  const [copied, setCopied] = useState(false);
  const key = teams.join("\u0000");

  const check = useCallback(async () => {
    if (teams.length === 0) { setRows([]); setRowsErr(null); return; }
    try {
      setRows(await api.crestStatus(teams));
      setRowsErr(null);
    } catch (e) {
      setRowsErr(api.toUiError(e).message);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [key]);

  useEffect(() => { title.current?.focus(); }, []);
  useEffect(() => {
    let alive = true;
    api.crestFolder().then((f) => alive && setFolder(f), (e) => alive && setFolderErr(api.toUiError(e).message));
    return () => { alive = false; };
  }, []);
  useEffect(() => { void check(); }, [check]);
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => { if (e.key === "Escape") onClose(); };
    document.addEventListener("keydown", onKey);
    return () => document.removeEventListener("keydown", onKey);
  }, [onClose]);

  async function copy() {
    if (!folder) return;
    try {
      await navigator.clipboard.writeText(folder);
      setCopied(true);
    } catch {
      setCopied(false);
    }
  }

  return (
    <section className="guide" aria-labelledby="guide-title">
      <h2 id="guide-title" ref={title} tabIndex={-1}>Guia de escudos</h2>
      <p>
        O Bygfoot <b>não inclui escudos</b> de clubes: só as bandeiras dos países. Escudos são marcas registradas e têm
        direitos autorais, e este projeto não os distribui. Se você quiser ver os escudos no jogo, <b>você precisa adicionar</b>{" "}
        as imagens por conta própria, usando só imagens que você tenha o direito de usar. Sem o arquivo, o jogo mostra um
        emblema simples com as iniciais do clube.
      </p>

      <ol className="steps" aria-label="Passo a passo para adicionar escudos">
        <li><b>Reúna as imagens.</b> Um arquivo <b>PNG</b> por clube (fundo transparente fica melhor). Tamanho recomendado:
          <b> 256 × 256 pixels</b>; o jogo aceita de 16 a 1024 pixels por lado e até <b>512 KB</b>. Renomear um JPG para .png não funciona: o arquivo precisa ser um PNG de verdade.</li>
        <li><b>Dê o nome certo ao arquivo.</b> É o nome do clube em <b>minúsculas</b>, sem acentos, com hífen no lugar de espaços e pontos,
          mais <code>.png</code>. Exemplo: <code>K. Klaksvík</code> vira <code>k-klaksvik.png</code>. A tabela abaixo mostra o nome exato dos clubes da partida atual.</li>
        <li><b>Copie para a pasta de escudos.</b>{" "}
          {folder ? <><code>{folder}</code>{" "}<button type="button" onClick={copy}>Copiar caminho</button>{copied && <span role="status"> Caminho copiado.</span>}</> : folderErr ? null : "Descobrindo a pasta…"}
          {folderErr && <span role="alert" className="error">Não foi possível abrir a pasta de escudos: {folderErr}</span>}
          {" "}Cole os arquivos ali; não use atalhos (links), só cópias.</li>
        <li><b>Confira.</b> Clique em <b>Verificar de novo</b>: cada clube passa a aparecer como Encontrado, Faltando ou Inválido, com o motivo. Não precisa reiniciar o jogo.</li>
        <li><b>Se algo deu errado:</b> confira se o nome está idêntico ao da tabela, se a extensão é .png, se o arquivo tem menos de 512 KB e se a imagem é um PNG de verdade.</li>
      </ol>

      <h3>Clubes da partida atual</h3>
      {teams.length === 0 ? (
        <p>Simule uma partida para ver aqui o nome exato do arquivo de cada clube.</p>
      ) : (
        <table aria-label="Escudos dos clubes da partida">
          <thead><tr><th>Clube</th><th>Arquivo esperado</th><th>Situação</th></tr></thead>
          <tbody>
            {rows.map((r, i) => (
              <tr key={`${r.team}-${i}`}>
                <td><Crest team={r.team} info={r} size={22} /> {r.team}</td>
                <td><code>{r.file || "—"}</code></td>
                <td>{STATUS_TEXT[r.status]}{r.reason ? ` — ${r.reason}` : ""}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
      {rowsErr && <p role="alert" className="error">Não foi possível verificar os escudos: {rowsErr}</p>}

      <p className="guide-actions">
        <button type="button" onClick={() => void check()} disabled={teams.length === 0 && !rowsErr}>Verificar de novo</button>{" "}
        <button type="button" onClick={onClose}>Voltar</button>
      </p>
    </section>
  );
}
