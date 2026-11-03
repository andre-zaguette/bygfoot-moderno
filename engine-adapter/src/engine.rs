use crate::{parse_timeline, EngineError, Timeline};
use std::io::Read;
use std::path::PathBuf;
use std::process::{Child, Command, Stdio};
use std::time::{Duration, Instant};

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
    /// Remendos que compõem o binário. É DECLARATIVO: vai para `source.patches` sem
    /// ser conferido contra o binário (ver ADR-003).
    pub patches: Vec<String>,
    /// Prazo máximo da simulação; ao estourar, o motor e seus filhos são encerrados.
    pub timeout: Duration,
}

/// O CI mediu ~34 s por partida no Windows; o prazo dá folga larga.
pub const DEFAULT_TIMEOUT: Duration = Duration::from_secs(180);

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
    pub fn new(wrapper: PathBuf, binary: PathBuf, patches: Vec<String>) -> Self {
        Engine { wrapper, binary, patches, timeout: DEFAULT_TIMEOUT }
    }

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
        let mut cmd = Command::new("sh");
        cmd.arg(&self.wrapper)
            .args(["--format", "timeline-v1"])
            .args(["--seed", &p.seed.to_string()])
            .args(["--weeks", &p.weeks.to_string()])
            .args(["--country", &p.country])
            .args(["--team", &p.team])
            .env("BYGFOOT_BIN", &self.binary)
            .env("BYGFOOT_PATCHES", self.patches.join(","))
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped());
        #[cfg(unix)]
        {
            use std::os::unix::process::CommandExt;
            cmd.process_group(0); // grupo próprio: dá para encerrar sh + motor de uma vez
        }
        let (status, stdout, stderr) = run_with_deadline(cmd, self.timeout)?;
        if !status.success() {
            return Err(EngineError::EngineFailed {
                code: status.code().unwrap_or(-1),
                stderr: String::from_utf8_lossy(&stderr).trim().to_string(),
            });
        }
        let t = parse_timeline(&String::from_utf8_lossy(&stdout))?;
        check_source(&t, p)?;
        Ok(t)
    }
}

/// A timeline precisa descrever o pedido feito, não outro.
fn check_source(t: &Timeline, p: &SimulateParams) -> Result<(), EngineError> {
    let s = &t.source;
    if s.seed != p.seed || s.weeks != p.weeks || s.country != p.country || s.team != p.team {
        return Err(EngineError::InvalidTimeline(format!(
            "source não corresponde ao pedido (seed {}/{}, weeks {}/{}, country {}/{}, team {}/{})",
            s.seed, p.seed, s.weeks, p.weeks, s.country, p.country, s.team, p.team
        )));
    }
    Ok(())
}

fn run_with_deadline(
    mut cmd: Command,
    timeout: Duration,
) -> Result<(std::process::ExitStatus, Vec<u8>, Vec<u8>), EngineError> {
    let mut child = cmd
        .spawn()
        .map_err(|e| EngineError::Unavailable(format!("não consegui executar sh: {e}")))?;
    let mut out = child.stdout.take().expect("stdout em pipe");
    let mut err = child.stderr.take().expect("stderr em pipe");
    // lê em threads para o motor nunca travar com o pipe cheio
    let t_out = std::thread::spawn(move || {
        let mut v = Vec::new();
        let _ = out.read_to_end(&mut v);
        v
    });
    let t_err = std::thread::spawn(move || {
        let mut v = Vec::new();
        let _ = err.read_to_end(&mut v);
        v
    });
    let start = Instant::now();
    let status = loop {
        match child.try_wait() {
            Ok(Some(s)) => break s,
            Ok(None) if start.elapsed() >= timeout => {
                kill_tree(&mut child);
                let _ = child.wait();
                let _ = t_out.join();
                let _ = t_err.join();
                return Err(EngineError::Timeout(timeout));
            }
            Ok(None) => std::thread::sleep(Duration::from_millis(25)),
            Err(e) => return Err(EngineError::Unavailable(format!("falha ao esperar o motor: {e}"))),
        }
    };
    Ok((status, t_out.join().unwrap_or_default(), t_err.join().unwrap_or_default()))
}

/// Encerra o `sh` e os filhos dele (o motor). Sem isso o motor sobraria órfão.
fn kill_tree(child: &mut Child) {
    #[cfg(unix)]
    {
        // process_group(0): o pgid é o pid do sh; "kill -KILL -- -PGID" atinge o grupo todo
        let _ = Command::new("kill").args(["-KILL", "--", &format!("-{}", child.id())]).status();
    }
    #[cfg(windows)]
    {
        let _ = Command::new("taskkill").args(["/T", "/F", "/PID", &child.id().to_string()]).status();
    }
    let _ = child.kill();
}
