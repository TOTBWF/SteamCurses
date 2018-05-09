use std::process::*;
use std::env;

pub fn launch_game(app_id: &str) -> Child {
    Command::new("steam")
        .arg("-applaunch")
        .arg(app_id)
        .stdout(Stdio::null())
        .spawn()
        .expect("Failed to execute game")
}

pub fn launch_steam() -> Child {
    let bin_path = env::var("HOME").unwrap() + "/.steam/bin";
    let cmd = bin_path.clone() + "/steam";
    Command::new(cmd)
        .arg("-silent")
        .env("LD_PRELOAD", "/usr/lib32/steam_injector.so")
        .env("LD_LIBRARY_PATH", bin_path)
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
        .expect("Failed to execute steam")
}
