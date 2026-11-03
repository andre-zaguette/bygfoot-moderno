use crate::{parse_timeline, EngineError, Timeline};
use std::path::PathBuf;
use std::process::Command;

#[derive(Debug, Clone)]
pub struct SimulateParams {
    pub seed: u32,
    pub country: String,
    pub team: String,
    pub weeks: u32,
}

/// Onde estão o wrapper (`bf-headless`) e o binário do motor.
#[derive(Debug, Clone)]
pub struct Engine {
    pub wrapper: PathBuf,
    pub binary: PathBuf,
    /// Remendos que compõem o binário (vai para `source.patches`).
    pub patches: Vec<String>,
}

pub(crate) const MAX_WEEKS: u32 = 100;
pub(crate) const MAX_SEED: u32 = 2_147_483_647;

impl SimulateParams {
    pub(crate) fn validate(&self) -> Result<(), EngineError> {
        let bad = |m: &str| Err(EngineError::InvalidParams(m.to_string()));
        if self.seed > MAX_SEED {
            return bad("seed fora de 0..2147483647");
        }
        if self.weeks == 0 || self.weeks > MAX_WEEKS {
            return bad("weeks deve estar entre 1 e 100");
        }
        for v in [&self.team, &self.country] {
            if v.is_empty() || v.chars().any(|c| c.is_control() || c == '"' || c == '\\') {
                return bad("time/país vazio ou com aspas, barra invertida ou caractere de controle");
            }
        }
        Ok(())
    }
}

impl Engine {
    pub fn simulate(&self, p: &SimulateParams) -> Result<Timeline, EngineError> {
        p.validate()?;
        if !self.wrapper.is_file() {
            return Err(EngineError::Unavailable(format!(
                "wrapper não encontrado: {}",
                self.wrapper.display()
            )));
        }
        if !self.binary.is_file() {
            return Err(EngineError::Unavailable(format!(
                "binário do motor não encontrado: {}",
                self.binary.display()
            )));
        }
        let out = Command::new("sh")
            .arg(&self.wrapper)
            .args(["--format", "timeline-v1"])
            .args(["--seed", &p.seed.to_string()])
            .args(["--weeks", &p.weeks.to_string()])
            .args(["--country", &p.country])
            .args(["--team", &p.team])
            .env("BYGFOOT_BIN", &self.binary)
            .env("BYGFOOT_PATCHES", self.patches.join(","))
            .output()
            .map_err(|e| EngineError::Unavailable(format!("não consegui executar sh: {e}")))?;
        if !out.status.success() {
            return Err(EngineError::EngineFailed {
                code: out.status.code().unwrap_or(-1),
                stderr: String::from_utf8_lossy(&out.stderr).trim().to_string(),
            });
        }
        parse_timeline(&String::from_utf8_lossy(&out.stdout))
    }
}
