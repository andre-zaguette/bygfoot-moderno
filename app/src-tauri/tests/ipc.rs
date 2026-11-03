//! Chama o comando `simulate_match` pelo caminho de IPC do Tauri (serialização incluída).
//! O teste do motor real só roda com BYGFOOT_BIN apontando para o binário remendado.
use serde_json::json;
use tauri::ipc::{CallbackFn, InvokeBody};
use tauri::test::{get_ipc_response, mock_builder, INVOKE_KEY};
use tauri::webview::InvokeRequest;
use tauri::WebviewWindowBuilder;

fn call(body: serde_json::Value) -> Result<serde_json::Value, serde_json::Value> {
    let app = mock_builder()
        .invoke_handler(tauri::generate_handler![bygfoot_moderno_app_lib::commands::simulate_match])
        .build(tauri::generate_context!())
        .unwrap();
    let w = WebviewWindowBuilder::new(&app, "main", Default::default()).build().unwrap();
    get_ipc_response(
        &w,
        InvokeRequest {
            cmd: "simulate_match".into(),
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
