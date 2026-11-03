use engine_adapter::{parse_timeline, Engine, EngineError, SimulateParams};
use std::path::PathBuf;
use std::time::{Duration, Instant};

fn root() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).parent().unwrap().to_path_buf()
}

fn params() -> SimulateParams {
    SimulateParams { seed: 431199, country: "faroe_islands".into(), team: "V. Gotu".into(), weeks: 2 }
}

fn wrapper() -> PathBuf {
    root().join("spike/headless/bf-headless")
}

fn fake(dir: &str, script: &str) -> Engine {
    let d = std::env::temp_dir().join(format!("bf-adapter-{dir}-{}", std::process::id()));
    std::fs::create_dir_all(&d).unwrap();
    let w = d.join("fake-wrapper.sh");
    std::fs::write(&w, script).unwrap();
    let b = d.join("fake-bin");
    std::fs::write(&b, "").unwrap();
    Engine { wrapper: w, binary: b, patches: vec![], timeout: Duration::from_secs(60) }
}

#[test]
fn rejects_invalid_params_before_running_anything() {
    let e = Engine { wrapper: "/nao/existe".into(), binary: "/nao/existe".into(), patches: vec![], timeout: Duration::from_secs(60) };
    for p in [
        SimulateParams { weeks: 0, ..params() },
        SimulateParams { weeks: 101, ..params() },
        SimulateParams { seed: 2_147_483_648, ..params() },
        SimulateParams { team: "a\"b".into(), ..params() },
        SimulateParams { team: "".into(), ..params() },
        SimulateParams { country: "x\\y".into(), ..params() },
    ] {
        assert!(matches!(e.simulate(&p).unwrap_err(), EngineError::InvalidParams(_)), "{p:?}");
    }
}

#[test]
fn reports_missing_engine() {
    let e = Engine { wrapper: wrapper(), binary: "/nao/existe".into(), patches: vec![], timeout: Duration::from_secs(60) };
    assert!(matches!(e.simulate(&params()).unwrap_err(), EngineError::Unavailable(_)));
    let e = Engine { wrapper: "/nao/existe".into(), binary: "/bin/sh".into(), patches: vec![], timeout: Duration::from_secs(60) };
    assert!(matches!(e.simulate(&params()).unwrap_err(), EngineError::Unavailable(_)));
}

#[test]
fn maps_engine_failure_with_code_and_stderr() {
    let e = fake("fail", "echo 'time não encontrado' >&2\nexit 3\n");
    match e.simulate(&params()).unwrap_err() {
        EngineError::EngineFailed { code, stderr } => {
            assert_eq!(code, 3);
            assert!(stderr.contains("time não encontrado"), "{stderr}");
        }
        other => panic!("{other:?}"),
    }
}

#[test]
fn maps_unreadable_output() {
    let e = fake("garbage", "echo 'isso não é json'\n");
    assert!(matches!(e.simulate(&params()).unwrap_err(), EngineError::BadOutput(_)));
}

/// Teste com o motor real; só roda quando BYGFOOT_BIN aponta para o binário REMENDADO.
#[test]
fn real_engine_matches_the_contract_sample() {
    let Ok(bin) = std::env::var("BYGFOOT_BIN") else {
        eprintln!("BYGFOOT_BIN ausente: teste do motor real ignorado");
        return;
    };
    let patches: Vec<String> = {
        let mut v: Vec<String> = std::fs::read_dir(root().join("patches"))
            .unwrap()
            .map(|e| e.unwrap().file_name().to_string_lossy().into_owned())
            .filter(|n| n.ends_with(".patch"))
            .collect();
        v.sort();
        v
    };
    let e = Engine { wrapper: wrapper(), binary: bin.into(), patches, timeout: Duration::from_secs(120) };
    let got = e.simulate(&params()).expect("motor real deve simular");
    let want = parse_timeline(
        &std::fs::read_to_string(root().join("tests/contract/timeline-v1.sample.json")).unwrap(),
    )
    .unwrap();
    assert_eq!(got, want, "a partida deve ser idêntica à amostra do contrato");
    // time inexistente: o motor falha com código 3
    let bad = SimulateParams { team: "Time Que Nao Existe".into(), ..params() };
    assert!(matches!(e.simulate(&bad).unwrap_err(), EngineError::EngineFailed { code: 3, .. }));
}

#[test]
fn kills_an_engine_that_hangs() {
    let mut e = fake("hang", "sleep 30\n");
    e.timeout = Duration::from_millis(800);
    let t0 = Instant::now();
    assert!(matches!(e.simulate(&params()).unwrap_err(), EngineError::Timeout(_)));
    assert!(t0.elapsed() < Duration::from_secs(10), "deve abortar no prazo, levou {:?}", t0.elapsed());
}

fn sample_wrapper(dir: &str) -> Engine {
    let sample = root().join("tests/contract/timeline-v1.sample.json");
    fake(dir, &format!("cat '{}'\n", sample.display()))
}

#[test]
fn accepts_output_that_matches_the_request() {
    let t = sample_wrapper("match").simulate(&params()).expect("source bate com o pedido");
    assert_eq!(t.source.seed, 431199);
}

#[test]
fn rejects_output_whose_source_differs_from_the_request() {
    let e = sample_wrapper("mismatch");
    for p in [
        SimulateParams { seed: 7, ..params() },
        SimulateParams { weeks: 3, ..params() },
        SimulateParams { team: "Outro".into(), ..params() },
        SimulateParams { country: "outro".into(), ..params() },
    ] {
        match e.simulate(&p).unwrap_err() {
            EngineError::InvalidTimeline(m) => assert!(m.contains("source"), "{m}"),
            other => panic!("{other:?}"),
        }
    }
}
