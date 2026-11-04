import type { CrestInfo } from "../types";

/** Iniciais para o emblema gerado: 1ª letra das duas primeiras palavras, ou as duas primeiras letras. */
export function initials(name: string): string {
  const words = name.split(/\s+/).filter((w) => /\p{L}/u.test(w));
  if (words.length === 0) return "?";
  const first = (w: string) => Array.from(w).find((c) => /\p{L}/u.test(c)) ?? "";
  if (words.length >= 2) return (first(words[0]) + first(words[1])).toUpperCase();
  return Array.from(words[0]).filter((c) => /\p{L}/u.test(c)).slice(0, 2).join("").toUpperCase();
}

/** Matiz (0..359) estável por nome: o mesmo clube tem sempre a mesma cor. */
export function hueOf(name: string): number {
  let h = 5381;
  for (const c of name) h = ((h << 5) + h + c.codePointAt(0)!) >>> 0;
  return h % 360;
}

/** Escudo do usuário, se houver; senão um emblema gerado (iniciais e cor), que não imita nenhum clube real. */
export default function Crest({ team, info, size = 28 }: { team: string; info?: CrestInfo; size?: number }) {
  if (info?.status === "found" && info.data_url) {
    return <img className="crest" src={info.data_url} width={size} height={size} alt={`Escudo de ${team}`} />;
  }
  const hue = hueOf(team);
  return (
    <svg className="crest" role="img" aria-label={`Emblema de ${team} (sem escudo)`} width={size} height={size} viewBox="0 0 32 32">
      <path d="M4 3h24v14c0 7-6 11-12 13C10 28 4 24 4 17z" fill={`hsl(${hue} 45% 28%)`} stroke="#fff" strokeWidth="1.5" />
      <text x="16" y="19" textAnchor="middle" fontSize="11" fontWeight="700" fill="#fff" fontFamily="system-ui, sans-serif">
        {initials(team)}
      </text>
    </svg>
  );
}
