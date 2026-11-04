//! Chama o comando `simulate_match` pelo caminho de IPC do Tauri (serialização incluída).
//! O teste do motor real só roda com BYGFOOT_BIN apontando para o binário remendado.
use serde_json::json;
use tauri::ipc::{CallbackFn, InvokeBody};
use tauri::test::{get_ipc_response, mock_builder, INVOKE_KEY};
use tauri::webview::InvokeRequest;
use tauri::WebviewWindowBuilder;

fn call(body: serde_json::Value) -> Result<serde_json::Value, serde_json::Value> {
    call_cmd("simulate_match", body)
}

fn call_cmd(cmd: &str, body: serde_json::Value) -> Result<serde_json::Value, serde_json::Value> {
    use bygfoot_moderno_app_lib::commands as c;
    let app = mock_builder()
        .invoke_handler(tauri::generate_handler![c::simulate_match, c::crest_folder, c::crest_status, c::catalog])
        .build(tauri::generate_context!())
        .unwrap();
    let w = WebviewWindowBuilder::new(&app, "main", Default::default()).build().unwrap();
    get_ipc_response(
        &w,
        InvokeRequest {
            cmd: cmd.into(),
            callback: CallbackFn(0),
            error: CallbackFn(1),
            url: (if cfg!(windows) { "http://tauri.localhost" } else { "tauri://localhost" }).parse().unwrap(),
            body: InvokeBody::Json(body),
            headers: Default::default(),
            invoke_key: INVOKE_KEY.to_string(),
        },
    )
    .map(|b| b.deserialize::<serde_json::Value>().unwrap())
    .map_err(|e| e)
}

#[test]
fn invalid_params_come_back_as_typed_error() {
    let err = call(json!({"seed": 1, "country": "faroe_islands", "team": "V. Gotu", "weeks": 0})).unwrap_err();
    assert_eq!(err["kind"], "invalid_params", "{err}");
}

#[test]
fn real_engine_through_ipc_gives_the_contract_match() {
    if std::env::var_os("BYGFOOT_BIN").is_none() {
        eprintln!("BYGFOOT_BIN ausente: teste do motor real ignorado");
        return;
    }
    let ok = call(json!({"seed": 431199, "country": "faroe_islands", "team": "V. Gotu", "weeks": 2})).unwrap();
    let want: serde_json::Value = serde_json::from_str(include_str!("../../../tests/contract/timeline-v1.sample.json")).unwrap();
    assert_eq!(ok, want, "o JSON que a interface recebe deve ser igual à amostra do contrato");
    let err = call(json!({"seed": 1, "country": "faroe_islands", "team": "Nao Existe", "weeks": 2})).unwrap_err();
    assert_eq!(err["kind"], "engine_failed", "{err}");
}

#[test]
fn acl_denies_commands_that_are_not_allowed() {
    // privilégio mínimo: nem os comandos "core" padrão do Tauri (ex.: caminhos do sistema) são liberados
    for cmd in ["plugin:path|resolve_directory", "plugin:app|version", "plugin:window|title"] {
        let err = call_cmd(cmd, json!({"directory": 1, "label": "main"})).unwrap_err();
        assert!(err.to_string().contains("not allowed"), "{cmd}: {err}");
    }
}

fn crest_dir_for_test() -> std::path::PathBuf {
    // a pasta é global ao processo (variável de ambiente): um único valor para todos os testes deste arquivo
    let d = std::env::temp_dir().join(format!("bfm-ipc-escudos-{}", std::process::id()));
    std::env::set_var("BYGFOOT_ESCUDOS", &d);
    d
}

#[test]
fn crest_folder_creates_and_returns_the_user_folder() {
    let d = crest_dir_for_test();
    let _ = std::fs::remove_dir_all(&d);
    let got = call_cmd("crest_folder", json!({})).unwrap();
    assert_eq!(got, json!(d.to_string_lossy()));
    assert!(d.is_dir());
}

#[test]
fn crest_status_reports_missing_found_and_invalid_through_ipc() {
    let d = crest_dir_for_test();
    std::fs::create_dir_all(&d).unwrap();
    let mut png = vec![0x89, b'P', b'N', b'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13, b'I', b'H', b'D', b'R'];
    png.extend(256u32.to_be_bytes());
    png.extend(256u32.to_be_bytes());
    std::fs::write(d.join("santos.png"), &png).unwrap();
    std::fs::write(d.join("flamengo.png"), b"nao e png nenhum aqui, mesmo").unwrap();
    let got = call_cmd("crest_status", json!({"teams": ["Santos", "Flamengo", "Paysandu"]})).unwrap();
    let st: Vec<_> = got.as_array().unwrap().iter().map(|c| c["status"].as_str().unwrap().to_string()).collect();
    assert_eq!(st, ["found", "invalid", "missing"]);
    assert!(got[0]["data_url"].as_str().unwrap().starts_with("data:image/png;base64,"));
    assert_eq!(got[2]["file"], "paysandu.png");
    assert!(got[2]["data_url"].is_null());
}

#[test]
fn crest_status_rejects_too_many_or_too_long_names() {
    crest_dir_for_test();
    let many: Vec<String> = (0..65).map(|i| format!("T{i}")).collect();
    assert_eq!(call_cmd("crest_status", json!({"teams": many})).unwrap_err()["kind"], "invalid_params");
    assert_eq!(call_cmd("crest_status", json!({"teams": ["x".repeat(81)]})).unwrap_err()["kind"], "invalid_params");
}

#[test]
fn catalog_through_ipc_lists_the_bygfoot_database() {
    if !std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../upstream/support_files").is_dir() {
        return;
    }
    let got = call_cmd("catalog", json!({})).unwrap();
    let europe = got["continents"].as_array().unwrap().iter().find(|c| c["id"] == "europe").unwrap();
    let fo = europe["countries"].as_array().unwrap().iter().find(|c| c["sid"] == "faroe_islands").unwrap();
    assert_eq!(fo["leagues"][0]["sid"], "faroe_islands1");
    assert_eq!(fo["leagues"][0]["firstWeek"], 2, "campos em camelCase para a interface");
    assert!(fo["leagues"][0]["teams"].as_array().unwrap().iter().any(|t| t == "V. Gotu"));
}
