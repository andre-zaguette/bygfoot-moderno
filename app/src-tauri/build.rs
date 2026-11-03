fn main() {
    // Privilégio mínimo: só os comandos listados aqui podem ser chamados pela interface.
    tauri_build::try_build(
        tauri_build::Attributes::new()
            .app_manifest(tauri_build::AppManifest::new().commands(&["simulate_match"])),
    )
    .expect("falha no tauri-build");

    // Windows/MSVC: os executáveis de teste de integração não recebem o manifesto do
    // app e o Tauri falha ao carregar (STATUS_ENTRYPOINT_NOT_FOUND). Embute o mesmo
    // requisito (Common Controls v6) só nos testes.
    if std::env::var("CARGO_CFG_TARGET_ENV").as_deref() == Ok("msvc") {
        let manifest = std::path::Path::new(&std::env::var("CARGO_MANIFEST_DIR").unwrap())
            .join("windows-test.manifest");
        println!("cargo:rustc-link-arg-tests=/MANIFEST:EMBED");
        println!("cargo:rustc-link-arg-tests=/MANIFESTINPUT:{}", manifest.display());
        println!("cargo:rerun-if-changed=windows-test.manifest");
    }
}
