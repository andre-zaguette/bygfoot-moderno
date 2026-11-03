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
    pub commentary: String,
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
    if last.result != t.match_info.final_score {
        return Err(EngineError::InvalidTimeline(format!(
            "último result {:?} difere de match.final {:?}",
            last.result, t.match_info.final_score
        )));
    }
    if t.units.iter().any(|u| u.time > 3 || u.possession > 1) {
        return Err(EngineError::InvalidTimeline("time/possession fora do intervalo".into()));
    }
    Ok(t)
}
