//! Casca Tauri: expõe o motor do Bygfoot (via `engine-adapter`) à interface.
//! A interface nunca calcula resultado; só mostra a timeline devolvida pelo motor.

use engine_adapter::{Engine, EngineError, SimulateParams, Timeline};
use serde::Serialize;
use std::path::{Path, PathBuf};

/// Erro serializável para a interface: `kind` estável (para a UI decidir), `message` em português.
#[derive(Debug, Serialize, PartialEq)]
pub struct UiError {
    pub kind: &'static str,
    pub message: String,
}

impl From<EngineError> for UiError {
    fn from(e: EngineError) -> Self {
        let kind = match &e {
            EngineError::InvalidParams(_) => "invalid_params",
            EngineError::Unavailable(_) => "engine_unavailable",
            EngineError::EngineFailed { .. } => "engine_failed",
            EngineError::BadOutput(_) => "bad_output",
            EngineError::InvalidTimeline(_) => "invalid_timeline",
        };
        UiError { kind, message: e.to_string() }
    }
}

/// Monta o `Engine` a partir da raiz do repositório. Modo de DESENVOLVIMENTO:
/// o binário vem de `BYGFOOT_BIN` ou de `build/patched/bygfoot`. Empacotar o motor
/// dentro do app é trabalho da Fase 6.
/// Binário padrão do motor remendado (modo de desenvolvimento).
const DEFAULT_BIN: &str = if cfg!(windows) { "build/patched/bygfoot.exe" } else { "build/patched/bygfoot" };

pub fn engine_from(root: &Path, bin_env: Option<PathBuf>) -> Engine {
    let mut patches: Vec<String> = std::fs::read_dir(root.join("patches"))
        .map(|rd| {
            rd.filter_map(|e| e.ok())
                .map(|e| e.file_name().to_string_lossy().into_owned())
                .filter(|n| n.ends_with(".patch"))
                .collect()
        })
        .unwrap_or_default();
    patches.sort();
    Engine {
        wrapper: root.join("spike/headless/bf-headless"),
        binary: bin_env.unwrap_or_else(|| root.join(DEFAULT_BIN)),
        patches,
    }
}

fn repo_root() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../..").canonicalize().unwrap_or_else(|_| PathBuf::from("."))
}

pub mod commands {
    use super::*;

    #[tauri::command]
    pub async fn simulate_match(seed: u32, country: String, team: String, weeks: u32) -> Result<Timeline, UiError> {
        let engine = engine_from(&repo_root(), std::env::var_os("BYGFOOT_BIN").map(PathBuf::from));
        let params = SimulateParams { seed, country, team, weeks };
        // o motor leva alguns segundos: fora da thread da interface
        tauri::async_runtime::spawn_blocking(move || engine.simulate(&params))
            .await
            .map_err(|e| UiError { kind: "internal", message: format!("tarefa interrompida: {e}") })?
            .map_err(UiError::from)
    }
}

pub fn run() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![commands::simulate_match])
        .run(tauri::generate_context!())
        .expect("erro ao iniciar o Bygfoot Moderno");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn maps_every_engine_error_to_a_stable_kind() {
        let cases = [
            (EngineError::InvalidParams("x".into()), "invalid_params"),
            (EngineError::Unavailable("x".into()), "engine_unavailable"),
            (EngineError::EngineFailed { code: 3, stderr: "s".into() }, "engine_failed"),
            (EngineError::BadOutput("x".into()), "bad_output"),
            (EngineError::InvalidTimeline("x".into()), "invalid_timeline"),
        ];
        for (err, kind) in cases {
            assert_eq!(UiError::from(err).kind, kind);
        }
    }

    #[test]
    fn engine_failure_message_keeps_code_and_stderr() {
        let m = UiError::from(EngineError::EngineFailed { code: 3, stderr: "time não encontrado".into() }).message;
        assert!(m.contains('3') && m.contains("time não encontrado"), "{m}");
    }

    #[test]
    fn engine_paths_follow_the_repo_layout_and_env_override() {
        let root = repo_root();
        let e = engine_from(&root, None);
        assert!(e.wrapper.ends_with("spike/headless/bf-headless") && e.wrapper.is_file());
        assert!(e.binary.ends_with(DEFAULT_BIN));
        assert_eq!(e.patches.len(), 3, "os 3 remendos de patches/ devem ser declarados");
        assert!(e.patches.windows(2).all(|w| w[0] <= w[1]), "em ordem");
        let e2 = engine_from(&root, Some("/x/motor".into()));
        assert_eq!(e2.binary, PathBuf::from("/x/motor"));
    }
}
