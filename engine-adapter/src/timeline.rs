use crate::EngineError;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(deny_unknown_fields, rename_all = "camelCase")]
pub struct Timeline {
    pub schema_version: u32,
    pub source: Source,
    #[serde(rename = "match")]
    pub match_info: MatchInfo,
    pub units: Vec<Unit>,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(deny_unknown_fields, rename_all = "camelCase")]
pub struct Source {
    pub engine: String,
    pub upstream_commit: String,
    pub seed: u32,
    pub country: String,
    pub team: String,
    pub weeks: u32,
    pub patches: Vec<String>,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(deny_unknown_fields, rename_all = "camelCase")]
pub struct MatchInfo {
    pub fixture_id: i64,
    pub home: String,
    pub away: String,
    #[serde(rename = "final")]
    pub final_score: [i32; 2],
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Unit {
    /// -1 = o motor não deu minuto próprio a este lance.
    pub minute: i32,
    /// 0 1º tempo, 1 2º tempo, 2 prorrogação, 3 pênaltis.
    pub time: u8,
    /// 0 casa, 1 fora.
    pub possession: u8,
    pub area: i32,
    pub result: [i32; 2],
    pub event: Event,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Event {
    #[serde(rename = "type")]
    pub kind: i32,
    pub team: i32,
    /// Opcional no contrato: o extrator omite quando o motor não dá texto.
    #[serde(skip_serializing_if = "Option::is_none", default)]
    pub commentary: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none", default)]
    pub player: Option<Player>,
    #[serde(skip_serializing_if = "Option::is_none", default)]
    pub player2: Option<Player>,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Player {
    pub id: i64,
    pub name: String,
}

/// Lê e valida a saída `--format timeline-v1`. Verifica o formato (campos
/// desconhecidos são recusados) e as invariantes que o schema JSON não cobre.
pub fn parse_timeline(text: &str) -> Result<Timeline, EngineError> {
    let t: Timeline =
        serde_json::from_str(text).map_err(|e| EngineError::BadOutput(e.to_string()))?;
    if t.schema_version != 1 {
        return Err(EngineError::InvalidTimeline(format!(
            "schemaVersion {} não suportado (esperado 1)",
            t.schema_version
        )));
    }
    let last = t
        .units
        .last()
        .ok_or_else(|| EngineError::InvalidTimeline("units vazio".into()))?;
    let bad = |m: String| Err(EngineError::InvalidTimeline(m));
    if t.source.engine != "bygfoot" {
        return bad(format!("source.engine inesperado: {}", t.source.engine));
    }
    if !t.source.commit_is_valid() {
        return bad(format!("source.upstreamCommit deve ter 40 hex: {}", t.source.upstream_commit));
    }
    if t.match_info.final_score.iter().any(|g| *g < 0) {
        return bad("placar final negativo".into());
    }
    let mut prev = [0, 0];
    for (i, u) in t.units.iter().enumerate() {
        if u.time > 3 {
            return bad(format!("lance {i}: time {} fora de 0..3", u.time));
        }
        if u.possession > 1 {
            return bad(format!("lance {i}: possession {} fora de 0..1", u.possession));
        }
        if !(0..=2).contains(&u.area) {
            return bad(format!("lance {i}: area {} fora de 0..2", u.area));
        }
        if u.minute < -1 {
            return bad(format!("lance {i}: minute {} abaixo de -1", u.minute));
        }
        if !(0..=40).contains(&u.event.kind) {
            return bad(format!("lance {i}: event.type {} fora de 0..40", u.event.kind));
        }
        if !(-1..=1).contains(&u.event.team) {
            return bad(format!("lance {i}: event.team {} fora de -1..1", u.event.team));
        }
        if u.result.iter().any(|g| *g < 0) {
            return bad(format!("lance {i}: placar negativo"));
        }
        if u.result != prev {
            let goal = u.event.kind == GOAL || u.event.kind == OWN_GOAL;
            if !goal && u.time != 3 {
                return bad(format!("placar mudou no lance {i} sem gol (evento {})", u.event.kind));
            }
            if u.time != 3 && (u.result[0] + u.result[1]) - (prev[0] + prev[1]) != 1 {
                return bad(format!("gol no lance {i} não somou 1"));
            }
        }
        prev = u.result;
    }
    if last.result != t.match_info.final_score {
        return bad(format!(
            "último result {:?} difere de match.final {:?}",
            last.result, t.match_info.final_score
        ));
    }
    if t.units[0].event.kind != START_MATCH {
        return bad("o primeiro lance deve ser START_MATCH (1)".into());
    }
    if last.event.kind != END_MATCH {
        return bad("o último lance deve ser END_MATCH (4)".into());
    }
    Ok(t)
}

// LiveGameEventType de upstream/src/live_game_struct.h
const START_MATCH: i32 = 1;
const END_MATCH: i32 = 4;
const GOAL: i32 = 10;
const OWN_GOAL: i32 = 11;

impl Source {
    /// O commit do upstream tem 40 caracteres hexadecimais minúsculos?
    fn commit_is_valid(&self) -> bool {
        let c = self.upstream_commit.as_bytes();
        c.len() == 40 && c.iter().all(|b| b.is_ascii_digit() || (b'a'..=b'f').contains(b))
    }
}
