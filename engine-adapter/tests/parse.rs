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
