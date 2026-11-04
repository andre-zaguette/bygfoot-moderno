//! Escudos de clubes: o Bygfoot não traz nenhum. O usuário coloca PNGs numa pasta local
//! (`<dados do app>/escudos/<slug>.png`); nada é distribuído por este projeto.
//! Este módulo só LÊ a pasta, valida o arquivo e devolve a imagem como data URL.

use serde::Serialize;
use std::path::Path;

pub const MAX_BYTES: u64 = 512 * 1024;
pub const MAX_SIDE: u32 = 1024;
pub const MIN_SIDE: u32 = 16;

#[derive(Debug, Serialize, PartialEq)]
pub struct CrestInfo {
    pub team: String,
    pub slug: String,
    /// Nome do arquivo esperado, p.ex. `k-klaksvik.png`.
    pub file: String,
    /// "found" | "missing" | "invalid"
    pub status: &'static str,
    pub reason: Option<String>,
    pub data_url: Option<String>,
}

fn fold(c: char) -> &'static str {
    match c {
        'à' | 'á' | 'â' | 'ã' | 'ä' | 'å' | 'ă' | 'ą' => "a",
        'ç' | 'ć' | 'č' => "c",
        'ď' | 'đ' | 'ð' => "d",
        'è' | 'é' | 'ê' | 'ë' | 'ě' | 'ę' | 'ė' => "e",
        'ğ' => "g",
        'ì' | 'í' | 'î' | 'ï' | 'ı' => "i",
        'ł' => "l",
        'ñ' | 'ń' | 'ň' => "n",
        'ò' | 'ó' | 'ô' | 'õ' | 'ö' | 'ø' | 'ő' => "o",
        'ř' => "r",
        'ś' | 'š' | 'ş' => "s",
        'ť' | 'ţ' => "t",
        'ù' | 'ú' | 'û' | 'ü' | 'ů' | 'ű' => "u",
        'ý' | 'ÿ' => "y",
        'ź' | 'ż' | 'ž' => "z",
        'ß' => "ss",
        'æ' => "ae",
        'œ' => "oe",
        'þ' => "th",
        _ => "",
    }
}

/// Nome de arquivo derivado do nome do clube: minúsculas, sem acentos, só `a-z0-9` e `-`.
pub fn slug(name: &str) -> String {
    let mut out = String::new();
    let mut sep = false;
    for lc in name.chars().flat_map(char::to_lowercase) {
        let piece: String = if lc.is_ascii_alphanumeric() { lc.to_string() } else { fold(lc).to_string() };
        if piece.is_empty() {
            sep = true;
            continue;
        }
        if sep && !out.is_empty() {
            out.push('-');
        }
        sep = false;
        out.push_str(&piece);
    }
    out
}

pub fn base64(bytes: &[u8]) -> String {
    const T: &[u8; 64] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    let mut out = String::with_capacity(bytes.len().div_ceil(3) * 4);
    for ch in bytes.chunks(3) {
        let n = (ch[0] as u32) << 16 | (*ch.get(1).unwrap_or(&0) as u32) << 8 | *ch.get(2).unwrap_or(&0) as u32;
        out.push(T[(n >> 18) as usize & 63] as char);
        out.push(T[(n >> 12) as usize & 63] as char);
        out.push(if ch.len() > 1 { T[(n >> 6) as usize & 63] as char } else { '=' });
        out.push(if ch.len() > 2 { T[n as usize & 63] as char } else { '=' });
    }
    out
}

fn info(team: &str, slug: String, status: &'static str, reason: Option<String>, data_url: Option<String>) -> CrestInfo {
    let file = if slug.is_empty() { String::new() } else { format!("{slug}.png") };
    CrestInfo { team: team.to_string(), slug, file, status, reason, data_url }
}

/// Lê e valida `<dir>/<slug>.png` para um clube.
pub fn crest_info(dir: &Path, team: &str) -> CrestInfo {
    let sl = slug(team);
    let bad = |sl: String, why: &str| info(team, sl, "invalid", Some(why.to_string()), None);
    if sl.is_empty() {
        return bad(sl, "o nome do clube não tem letras ou números para formar o nome do arquivo");
    }
    let path = dir.join(format!("{sl}.png"));
    let meta = match std::fs::symlink_metadata(&path) {
        Ok(m) => m,
        Err(_) => return info(team, sl, "missing", None, None),
    };
    if meta.file_type().is_symlink() {
        return bad(sl, "atalhos (links simbólicos) não são aceitos; copie o arquivo para a pasta");
    }
    if !meta.is_file() {
        return bad(sl, "o nome existe, mas não é um arquivo");
    }
    if meta.len() > MAX_BYTES {
        return bad(sl, "arquivo maior que 512 KB; reduza a imagem");
    }
    let bytes = match std::fs::read(&path) {
        Ok(b) => b,
        Err(_) => return bad(sl, "não foi possível ler o arquivo"),
    };
    if bytes.len() < 24 || bytes[..8] != [0x89, b'P', b'N', b'G', 0x0D, 0x0A, 0x1A, 0x0A] || &bytes[12..16] != b"IHDR" {
        return bad(sl, "o arquivo não é um PNG válido");
    }
    let w = u32::from_be_bytes([bytes[16], bytes[17], bytes[18], bytes[19]]);
    let h = u32::from_be_bytes([bytes[20], bytes[21], bytes[22], bytes[23]]);
    if !(MIN_SIDE..=MAX_SIDE).contains(&w) || !(MIN_SIDE..=MAX_SIDE).contains(&h) {
        return bad(sl, "tamanho fora do aceito: use entre 16 e 1024 pixels por lado (recomendado 256x256)");
    }
    let url = format!("data:image/png;base64,{}", base64(&bytes));
    info(team, sl, "found", None, Some(url))
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;

    fn png(w: u32, h: u32, extra: usize) -> Vec<u8> {
        let mut v = vec![0x89, b'P', b'N', b'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13, b'I', b'H', b'D', b'R'];
        v.extend(w.to_be_bytes());
        v.extend(h.to_be_bytes());
        v.extend(vec![0u8; extra]);
        v
    }
    fn tmp(name: &str) -> std::path::PathBuf {
        let d = std::env::temp_dir().join(format!("bfm-crests-{}-{name}", std::process::id()));
        let _ = fs::remove_dir_all(&d);
        fs::create_dir_all(&d).unwrap();
        d
    }

    #[test]
    fn slug_remove_acentos_pontos_e_espacos() {
        assert_eq!(slug("K. Klaksvík"), "k-klaksvik");
        assert_eq!(slug("São Paulo"), "sao-paulo");
        assert_eq!(slug("  Real   Madrid CF "), "real-madrid-cf");
        assert_eq!(slug("Malmö FF"), "malmo-ff");
        assert_eq!(slug("A/B\\..\\C"), "a-b-c");
        assert_eq!(slug("Çaykur Rizespor"), "caykur-rizespor");
    }

    #[test]
    fn slug_de_nome_sem_letras_e_vazio() {
        assert_eq!(slug("…!?"), "");
        assert_eq!(slug("../../etc/passwd"), "etc-passwd");
    }

    #[test]
    fn base64_confere_com_vetores_conhecidos() {
        assert_eq!(base64(b""), "");
        assert_eq!(base64(b"f"), "Zg==");
        assert_eq!(base64(b"fo"), "Zm8=");
        assert_eq!(base64(b"foo"), "Zm9v");
        assert_eq!(base64(b"foobar"), "Zm9vYmFy");
    }

    #[test]
    fn arquivo_ausente_e_missing_com_o_nome_esperado() {
        let d = tmp("ausente");
        let c = crest_info(&d, "K. Klaksvík");
        assert_eq!((c.status, c.file.as_str(), c.slug.as_str()), ("missing", "k-klaksvik.png", "k-klaksvik"));
        assert!(c.data_url.is_none());
    }

    #[test]
    fn png_valido_vira_data_url() {
        let d = tmp("valido");
        fs::write(d.join("santos.png"), png(256, 256, 10)).unwrap();
        let c = crest_info(&d, "Santos");
        assert_eq!(c.status, "found", "{:?}", c.reason);
        assert!(c.data_url.unwrap().starts_with("data:image/png;base64,iVBORw0KGgo"));
    }

    #[test]
    fn rejeita_o_que_nao_e_png() {
        let d = tmp("naopng");
        fs::write(d.join("santos.png"), b"GIF89a....................").unwrap();
        let c = crest_info(&d, "Santos");
        assert_eq!(c.status, "invalid");
        assert!(c.reason.unwrap().contains("PNG"));
        assert!(c.data_url.is_none());
    }

    #[test]
    fn rejeita_arquivo_grande_demais_e_dimensoes_fora_do_limite() {
        let d = tmp("limites");
        fs::write(d.join("a.png"), png(256, 256, MAX_BYTES as usize)).unwrap();
        assert_eq!(crest_info(&d, "A").status, "invalid");
        fs::write(d.join("b.png"), png(2048, 2048, 10)).unwrap();
        assert_eq!(crest_info(&d, "B").status, "invalid");
        fs::write(d.join("c.png"), png(8, 8, 10)).unwrap();
        assert_eq!(crest_info(&d, "C").status, "invalid");
        fs::write(d.join("d.png"), png(0, 0, 10)).unwrap();
        assert_eq!(crest_info(&d, "D").status, "invalid");
    }

    #[test]
    fn png_truncado_nao_derruba() {
        let d = tmp("truncado");
        fs::write(d.join("santos.png"), [0x89, b'P', b'N', b'G']).unwrap();
        assert_eq!(crest_info(&d, "Santos").status, "invalid");
    }

    #[test]
    fn nome_sem_letras_nunca_toca_o_disco() {
        let d = tmp("vazio");
        fs::write(d.join(".png"), png(256, 256, 1)).unwrap();
        let c = crest_info(&d, "…");
        assert_eq!(c.status, "invalid");
        assert!(c.data_url.is_none());
    }

    #[cfg(unix)]
    #[test]
    fn link_simbolico_e_recusado() {
        let d = tmp("symlink");
        let fora = tmp("symlink-fora");
        fs::write(fora.join("segredo.png"), png(256, 256, 1)).unwrap();
        std::os::unix::fs::symlink(fora.join("segredo.png"), d.join("santos.png")).unwrap();
        let c = crest_info(&d, "Santos");
        assert_eq!(c.status, "invalid");
        assert!(c.data_url.is_none());
    }
}
