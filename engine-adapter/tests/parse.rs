use engine_adapter::{parse_timeline, EngineError};

const SAMPLE: &str = include_str!("../../tests/contract/timeline-v1.sample.json");

fn sample_value() -> serde_json::Value {
    serde_json::from_str(SAMPLE).unwrap()
}

#[test]
fn parses_the_contract_sample() {
    let t = parse_timeline(SAMPLE).expect("amostra do contrato deve ser válida");
    assert_eq!(t.schema_version, 1);
    assert_eq!(t.source.seed, 431199);
    assert_eq!(t.match_info.home, "N. Runavík");
    assert_eq!(t.match_info.final_score, [1, 2]);
    assert_eq!(t.units.len(), 118);
    assert_eq!(t.units.last().unwrap().result, [1, 2]);
}

#[test]
fn rejects_unknown_schema_version() {
    let mut v = sample_value();
    v["schemaVersion"] = 2.into();
    let err = parse_timeline(&v.to_string()).unwrap_err();
    assert!(matches!(err, EngineError::InvalidTimeline(_)), "{err:?}");
}

#[test]
fn rejects_last_result_different_from_final() {
    let mut v = sample_value();
    v["match"]["final"] = serde_json::json!([9, 9]);
    let err = parse_timeline(&v.to_string()).unwrap_err();
    assert!(matches!(err, EngineError::InvalidTimeline(ref m) if m.contains("final")), "{err:?}");
}

#[test]
fn rejects_unknown_field() {
    let mut v = sample_value();
    v["extra"] = 1.into();
    assert!(parse_timeline(&v.to_string()).is_err());
}

#[test]
fn rejects_empty_units() {
    let mut v = sample_value();
    v["units"] = serde_json::json!([]);
    assert!(matches!(
        parse_timeline(&v.to_string()).unwrap_err(),
        EngineError::InvalidTimeline(_)
    ));
}

#[test]
fn rejects_garbage_output() {
    assert!(matches!(parse_timeline("não é json").unwrap_err(), EngineError::BadOutput(_)));
}

fn with_unit(idx: usize, f: impl FnOnce(&mut serde_json::Value)) -> String {
    let mut v = sample_value();
    f(&mut v["units"][idx]);
    v.to_string()
}

fn invalid(text: &str) -> String {
    match parse_timeline(text).unwrap_err() {
        EngineError::InvalidTimeline(m) => m,
        other => panic!("esperava InvalidTimeline, veio {other:?}"),
    }
}

#[test]
fn accepts_a_unit_without_commentary() {
    // o schema só exige type e team; o extrator omite commentary quando vazio
    let text = with_unit(3, |u| {
        u["event"].as_object_mut().unwrap().remove("commentary");
    });
    let t = parse_timeline(&text).expect("commentary é opcional no contrato");
    assert_eq!(t.units[3].event.commentary, None);
}

#[test]
fn rejects_values_outside_the_schema_ranges() {
    assert!(invalid(&with_unit(2, |u| u["area"] = 3.into())).contains("area"));
    assert!(invalid(&with_unit(2, |u| u["event"]["type"] = 41.into())).contains("type"));
    assert!(invalid(&with_unit(2, |u| u["event"]["team"] = 2.into())).contains("team"));
    assert!(invalid(&with_unit(2, |u| u["minute"] = (-2).into())).contains("minute"));
}

#[test]
fn rejects_wrong_engine_or_commit() {
    let mut v = sample_value();
    v["source"]["engine"] = "outro".into();
    assert!(invalid(&v.to_string()).contains("engine"));
    let mut v = sample_value();
    v["source"]["upstreamCommit"] = "xyz".into();
    assert!(invalid(&v.to_string()).contains("upstreamCommit"));
}

#[test]
fn rejects_negative_scores() {
    let mut v = sample_value();
    v["match"]["final"] = serde_json::json!([-1, 2]);
    let n = v["units"].as_array().unwrap().len();
    v["units"][n - 1]["result"] = serde_json::json!([-1, 2]);
    assert!(invalid(&v.to_string()).contains("negativ"));
}

#[test]
fn rejects_missing_start_or_end_event() {
    assert!(invalid(&with_unit(0, |u| u["event"]["type"] = 0.into())).contains("START"));
    let n = sample_value()["units"].as_array().unwrap().len();
    assert!(invalid(&with_unit(n - 1, |u| u["event"]["type"] = 0.into())).contains("END"));
}

#[test]
fn rejects_score_change_without_a_goal() {
    let mut v = sample_value();
    let units = v["units"].as_array().unwrap().clone();
    let i = (1..units.len() - 1)
        .find(|&i| ![10, 11].contains(&units[i]["event"]["type"].as_i64().unwrap()) && units[i]["time"] != 3)
        .unwrap();
    let prev = units[i - 1]["result"].clone();
    v["units"][i]["result"] = serde_json::json!([prev[0].as_i64().unwrap() + 1, prev[1]]);
    assert!(invalid(&v.to_string()).contains("gol"));
}
