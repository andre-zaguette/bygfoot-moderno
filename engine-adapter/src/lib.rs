//! Adaptador do motor do Bygfoot: roda o motor como processo separado
//! (via `bf-headless`) e devolve o contrato `MatchTimeline v1` validado.
//! O motor C nunca é ligado a este processo; o `upstream/` não é tocado.

mod engine;
mod timeline;

pub use engine::{Engine, SimulateParams, DEFAULT_TIMEOUT};
pub use timeline::{parse_timeline, Event, MatchInfo, Player, Source, Timeline, Unit};

#[derive(Debug, thiserror::Error)]
pub enum EngineError {
    /// Parâmetro recusado antes de chamar o motor.
    #[error("parâmetro inválido: {0}")]
    InvalidParams(String),
    /// O wrapper ou o binário do motor não existe.
    #[error("motor indisponível: {0}")]
    Unavailable(String),
    /// O motor rodou e falhou (ex.: time inexistente).
    #[error("o motor falhou (código {code}): {stderr}")]
    EngineFailed { code: i32, stderr: String },
    /// O motor não terminou no prazo e foi encerrado.
    #[error("o motor excedeu {0:?} e foi interrompido")]
    Timeout(std::time::Duration),
    /// A saída não é JSON do formato esperado.
    #[error("saída do motor ilegível: {0}")]
    BadOutput(String),
    /// É JSON, mas viola o contrato MatchTimeline v1.
    #[error("timeline fora do contrato: {0}")]
    InvalidTimeline(String),
}
