use std::process::*;

pub fn launch_game(app_id: &str) -> Child {
    Command::new("steam")
        .arg("-applaunch")
        .arg(app_id)
        // .arg(String::from("-applaunch ") + app_id)
        .stdout(Stdio::null())
        .spawn()
        .expect("Failed to execute game")
}

pub fn launch_steam() -> Child {
    Command::new("steam")
        .arg("-silent")
        // .env("LD_PRELOAD", "/usr/lib32/steam_injector.so")
        .stdout(Stdio::null())
        .spawn()
        .expect("Failed to execute steam")
}
