//! Catálogo da base de dados do Bygfoot: continentes, países, ligas, copas e clubes,
//! lido de `upstream/support_files/definitions` (os mesmos XML que o motor usa).
//! A leitura é tolerante: o próprio upstream tem XML quebrados (p.ex. `league_mexico3-1.xml`),
//! então usamos um extrator de tags simples em vez de um parser estrito.

use serde::Serialize;
use std::path::Path;

#[derive(Debug, Serialize, PartialEq, Clone)]
pub struct Catalog {
    pub continents: Vec<Continent>,
}
#[derive(Debug, Serialize, PartialEq, Clone)]
pub struct Continent {
    pub id: String,
    pub countries: Vec<Country>,
}
#[derive(Debug, Serialize, PartialEq, Clone)]
pub struct Country {
    /// Identificador que o motor recebe em `--country` (nome do arquivo `country_<sid>.xml`).
    pub sid: String,
    pub name: String,
    pub flag: Option<String>,
    pub leagues: Vec<League>,
    pub cups: Vec<Cup>,
}
#[derive(Debug, Serialize, PartialEq, Clone)]
#[serde(rename_all = "camelCase")]
pub struct League {
    pub sid: String,
    pub name: String,
    pub layer: Option<u32>,
    pub first_week: Option<u32>,
    pub teams: Vec<String>,
}
#[derive(Debug, Serialize, PartialEq, Clone)]
pub struct Cup {
    pub sid: String,
    pub name: String,
}

/// Modos de seleções que fecham o motor ORIGINAL com SIGSEGV (exit 139) ao simular; ficam fora do seletor.
/// Evidência: `bf-headless --country fifa|wc06|euro08` (ver ADR-006). Não é defeito do nosso código.
pub const ENGINE_CRASHES: &[&str] = &["fifa", "wc06", "euro08"];

fn read(p: &Path) -> Option<String> {
    std::fs::read(p).ok().map(|b| String::from_utf8_lossy(&b).into_owned())
}

fn decode(s: &str) -> String {
    s.replace("&lt;", "<").replace("&gt;", ">").replace("&quot;", "\"").replace("&apos;", "'").replace("&amp;", "&")
}

/// Todos os textos de `<tag>...</tag>` (tag sem atributos). Para no primeiro trecho incompleto.
fn tag_all(text: &str, tag: &str) -> Vec<String> {
    let (open, close) = (format!("<{tag}>"), format!("</{tag}>"));
    let mut out = Vec::new();
    let mut rest = text;
    while let Some(i) = rest.find(&open) {
        let after = &rest[i + open.len()..];
        let Some(j) = after.find(&close) else { break };
        out.push(decode(after[..j].trim()));
        rest = &after[j + close.len()..];
    }
    out
}

fn tag_first(text: &str, tag: &str) -> Option<String> {
    tag_all(text, tag).into_iter().next().filter(|s| !s.is_empty())
}

/// O motor recebe o nome do clube dentro de um JSON escrito à mão (ver `bf-headless`): sem aspas, barra invertida ou controles.
fn valid_team(name: &str) -> bool {
    !name.trim().is_empty() && name.chars().count() <= 80 && !name.chars().any(|c| c == '"' || c == '\\' || c.is_control())
}

fn sorted_dirs(p: &Path) -> Vec<std::path::PathBuf> {
    let mut v: Vec<_> = std::fs::read_dir(p).map(|rd| rd.filter_map(|e| e.ok()).map(|e| e.path()).filter(|p| p.is_dir()).collect()).unwrap_or_default();
    v.sort();
    v
}

fn load_country(dir: &Path) -> Option<Country> {
    let mut files: Vec<String> = std::fs::read_dir(dir).ok()?.filter_map(|e| e.ok()).map(|e| e.file_name().to_string_lossy().into_owned()).collect();
    files.sort();
    let cf = files.iter().find(|f| f.starts_with("country_") && f.ends_with(".xml"))?;
    let sid = cf.trim_start_matches("country_").trim_end_matches(".xml").to_string();
    if ENGINE_CRASHES.contains(&sid.as_str()) {
        return None;
    }
    let text = read(&dir.join(cf))?;
    let name = tag_first(&text, "name").map(|n| n.replace('_', " ")).unwrap_or_else(|| sid.clone());
    let leagues = tag_all(&text, "league")
        .into_iter()
        .filter_map(|lsid| {
            let lt = read(&dir.join(format!("league_{lsid}.xml")))?;
            let mut teams: Vec<String> = Vec::new();
            for t in tag_all(&lt, "team_name") {
                if valid_team(&t) && !teams.contains(&t) {
                    teams.push(t);
                }
            }
            Some(League {
                name: tag_first(&lt, "name").unwrap_or_else(|| lsid.clone()),
                layer: tag_first(&lt, "layer").and_then(|v| v.parse().ok()),
                first_week: tag_first(&lt, "first_week").and_then(|v| v.parse().ok()),
                sid: lsid,
                teams,
            })
        })
        .collect();
    let cups = tag_all(&text, "cup")
        .into_iter()
        .filter_map(|csid| {
            let ct = read(&dir.join(format!("cup_{csid}.xml")))?;
            Some(Cup { name: tag_first(&ct, "name").unwrap_or_else(|| csid.clone()), sid: csid })
        })
        .collect();
    Some(Country { sid, name, flag: tag_first(&text, "symbol"), leagues, cups })
}

pub fn load_catalog(support_files: &Path) -> Catalog {
    let mut continents = Vec::new();
    for cdir in sorted_dirs(&support_files.join("definitions")) {
        let mut countries: Vec<Country> = sorted_dirs(&cdir).iter().filter_map(|d| load_country(d)).collect();
        if countries.is_empty() {
            continue;
        }
        countries.sort_by_key(|c| c.name.to_lowercase());
        continents.push(Continent { id: cdir.file_name().unwrap().to_string_lossy().into_owned(), countries });
    }
    Catalog { continents }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;
    use std::path::PathBuf;

    fn tmp(name: &str) -> PathBuf {
        let d = std::env::temp_dir().join(format!("bfm-catalog-{}-{name}", std::process::id()));
        let _ = fs::remove_dir_all(&d);
        fs::create_dir_all(&d).unwrap();
        d
    }
    fn w(d: &Path, rel: &str, s: &str) {
        let p = d.join(rel);
        fs::create_dir_all(p.parent().unwrap()).unwrap();
        fs::write(p, s).unwrap();
    }

    #[test]
    fn le_pais_ligas_copas_e_clubes_em_ordem() {
        let d = tmp("basico");
        w(&d, "definitions/europe/pais_a/country_pais_a.xml",
          "<country><name>Pais_A</name><symbol>flag_a.png</symbol><leagues><league>pais_a1</league><league>pais_a2</league></leagues><cups><cup>pais_a_cup</cup></cups></country>");
        w(&d, "definitions/europe/pais_a/league_pais_a1.xml",
          "<league><sid>pais_a1</sid><name>Primeira</name><layer>1</layer><first_week>3</first_week><teams><team><team_name>Beta FC</team_name></team><team><team_name>Alfa &amp; Cia</team_name></team></teams></league>");
        w(&d, "definitions/europe/pais_a/league_pais_a2.xml",
          "<league><sid>pais_a2</sid><name>Segunda</name><layer>2</layer><teams><team><team_name>Gama</team_name></team></teams></league>");
        w(&d, "definitions/europe/pais_a/cup_pais_a_cup.xml", "<cup><sid>pais_a_cup</sid><name>Copa A</name></cup>");
        let c = load_catalog(&d);
        assert_eq!(c.continents.len(), 1);
        let p = &c.continents[0].countries[0];
        assert_eq!((c.continents[0].id.as_str(), p.sid.as_str(), p.name.as_str()), ("europe", "pais_a", "Pais A"));
        assert_eq!(p.flag.as_deref(), Some("flag_a.png"));
        assert_eq!(p.leagues.iter().map(|l| l.sid.as_str()).collect::<Vec<_>>(), ["pais_a1", "pais_a2"]);
        assert_eq!(p.leagues[0].teams, ["Beta FC", "Alfa & Cia"], "ordem do arquivo e entidades decodificadas");
        assert_eq!((p.leagues[0].layer, p.leagues[0].first_week), (Some(1), Some(3)));
        assert_eq!(p.leagues[1].first_week, None);
        assert_eq!(p.cups, [Cup { sid: "pais_a_cup".into(), name: "Copa A".into() }]);
    }

    #[test]
    fn sid_do_pais_vem_do_nome_do_arquivo_mesmo_sem_tag_sid() {
        let d = tmp("sid");
        w(&d, "definitions/north_america/mexico/country_mexico.xml", "<country><name>México</name><leagues><league>mexico1</league></leagues></country>");
        w(&d, "definitions/north_america/mexico/league_mexico1.xml", "<league><sid>mexico1</sid><name>Primera</name><teams><team><team_name>Pumas</team_name></team></teams></league>");
        let c = load_catalog(&d);
        assert_eq!(c.continents[0].countries[0].sid, "mexico");
        assert_eq!(c.continents[0].countries[0].name, "México");
    }

    #[test]
    fn xml_quebrado_nao_derruba_e_aproveita_o_que_da() {
        let d = tmp("quebrado");
        w(&d, "definitions/europe/x/country_x.xml", "<country><name>X</name><leagues><league>x1</league><league>x2</league><league>x3</league></leagues></country>");
        w(&d, "definitions/europe/x/league_x1.xml", "<league><sid>x1</sid><name>Um</name><teams><team><team_name>Time 1</team_name></team><team><team_name>Time 2</te");
        w(&d, "definitions/europe/x/league_x2.xml", "");
        // league_x3 não existe
        let c = load_catalog(&d);
        let ls = &c.continents[0].countries[0].leagues;
        assert_eq!(ls[0].teams, ["Time 1"], "só o que está completo");
        assert!(ls.iter().all(|l| l.sid != "x3"), "liga listada sem arquivo é ignorada");
    }

    #[test]
    fn clube_repetido_na_liga_aparece_uma_vez_e_nome_invalido_e_filtrado() {
        let d = tmp("dup");
        w(&d, "definitions/europe/y/country_y.xml", "<country><name>Y</name><leagues><league>y1</league></leagues></country>");
        w(&d, "definitions/europe/y/league_y1.xml",
          "<league><sid>y1</sid><name>L</name><teams><team><team_name>A</team_name></team><team><team_name>A</team_name></team><team><team_name>Com \"aspas\"</team_name></team><team><team_name>  </team_name></team></teams></league>");
        let c = load_catalog(&d);
        assert_eq!(c.continents[0].countries[0].leagues[0].teams, ["A"]);
    }

    #[test]
    fn pastas_sem_arquivo_de_pais_sao_ignoradas_e_a_ordem_e_estavel() {
        let d = tmp("ordem");
        w(&d, "definitions/miscellaneous/europe/cup_uefa.xml", "<cup><sid>u</sid><name>U</name></cup>");
        w(&d, "definitions/europe/zz/country_zz.xml", "<country><name>Zeta</name></country>");
        w(&d, "definitions/europe/aa/country_aa.xml", "<country><name>Alfa</name></country>");
        w(&d, "definitions/africa/bb/country_bb.xml", "<country><name>Beta</name></country>");
        let c = load_catalog(&d);
        assert_eq!(c.continents.iter().map(|x| x.id.as_str()).collect::<Vec<_>>(), ["africa", "europe"]);
        assert_eq!(c.continents[1].countries.iter().map(|x| x.name.as_str()).collect::<Vec<_>>(), ["Alfa", "Zeta"]);
    }

    #[test]
    fn modos_que_derrubam_o_motor_nao_entram_no_catalogo() {
        let d = tmp("crash");
        for sid in ["fifa", "wc06", "euro08", "eu"] {
            w(&d, &format!("definitions/miscellaneous/{sid}/country_{sid}.xml"), &format!("<country><name>{sid}</name><leagues><league>{sid}_l</league></leagues></country>"));
            w(&d, &format!("definitions/miscellaneous/{sid}/league_{sid}_l.xml"), "<league><sid>l</sid><name>L</name><teams><team><team_name>T</team_name></team></teams></league>");
        }
        let c = load_catalog(&d);
        let sids: Vec<&str> = c.continents[0].countries.iter().map(|x| x.sid.as_str()).collect();
        assert_eq!(sids, ["eu"], "fifa, wc06 e euro08 fecham o motor original (exit 139)");
    }

    #[test]
    fn pasta_inexistente_da_catalogo_vazio() {
        assert!(load_catalog(Path::new("/nao/existe/mesmo")).continents.is_empty());
    }

    fn real() -> Option<Catalog> {
        let p = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../upstream/support_files");
        p.is_dir().then(|| load_catalog(&p))
    }

    #[test]
    fn base_real_do_bygfoot_tem_os_paises_ligas_e_clubes_esperados() {
        let Some(c) = real() else { return };
        let countries: Vec<&Country> = c.continents.iter().flat_map(|x| &x.countries).collect();
        assert!(countries.len() >= 100, "{}", countries.len());
        let fo = countries.iter().find(|x| x.sid == "faroe_islands").unwrap();
        assert_eq!(fo.leagues.iter().map(|l| l.sid.as_str()).collect::<Vec<_>>(), ["faroe_islands1", "faroe_islands2", "faroe_islands3", "faroe_islands4"]);
        assert!(fo.leagues[0].teams.iter().any(|t| t == "V. Gotu"));
        assert_eq!(fo.leagues[0].first_week, Some(2));
        for bad in ENGINE_CRASHES {
            assert!(countries.iter().all(|x| x.sid != *bad), "{bad}");
        }
        let mx = countries.iter().find(|x| x.sid == "mexico").unwrap();
        assert_eq!(mx.leagues.len(), 7);
        assert!(mx.leagues[0].teams.len() >= 10);
        let teams: usize = countries.iter().flat_map(|x| &x.leagues).map(|l| l.teams.len()).sum();
        assert!(teams >= 8000, "{teams}");
        let cups: usize = countries.iter().map(|x| x.cups.len()).sum();
        assert!(cups >= 300, "{cups}");
    }
}
