fn main() {
    // Privilégio mínimo: só os comandos listados aqui podem ser chamados pela interface.
    tauri_build::try_build(
        tauri_build::Attributes::new()
            .app_manifest(tauri_build::AppManifest::new().commands(&["simulate_match"])),
    )
    .expect("falha no tauri-build");
}
