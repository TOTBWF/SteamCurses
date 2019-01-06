use std::process::*;
use std::path::Path;
use std::fs::{File, OpenOptions};

use config::*;
use game::*;

pub struct SteamClient {
    proc: Child,
    log_file: Option<File>,
    wine_prefix: Option<Box<Path>>
}

impl Drop for SteamClient {
    fn drop(&mut self) {
        self.proc.kill().unwrap();
        let output = mk_pipe(&self.log_file);
        let error = mk_pipe(&self.log_file);
        Command::new("steam")
            .arg("-shutdown")
            .stdout(output)
            .stderr(error)
            .spawn()
            .expect("[ERROR] Failed to execute game!");
    }
}

fn mk_pipe(log_file: &Option<File>) -> Stdio {
    match log_file {
        &Some(ref f) => Stdio::from(f.try_clone().unwrap()),
        &None => Stdio::null()
    }
}

impl SteamClient {
    pub fn new(config: Config) -> SteamClient {
        let log_file = config.log_path.map(|p| {
            OpenOptions::new()
                .write(true)
                .append(true)
                .create(true)
                .open(p.as_ref())
                .expect("[Error] Cannot open log file!")
        });
        let output = mk_pipe(&log_file);
        let error = mk_pipe(&log_file);

        let args = vec!["-silent"];

        let proc = Command::new("steam")
            .args(args)
            .stdout(output)
            .stderr(error)
            .spawn()
            .expect("[ERROR] Failed to execute steam!");
        SteamClient {
            proc: proc,
            log_file: log_file,
            wine_prefix: config.wine_prefix
        }
    }

    pub fn launch_game(&mut self, game: &Game) {
        let output = mk_pipe(&self.log_file);
        let error = mk_pipe(&self.log_file);
        match &game.game_type {
            GameType::Native => {
                Command::new("steam")
                    .arg("-applaunch")
                    .arg(&game.app_id)
                    .stdout(output)
                    .stderr(error)
                    .spawn()
                    .expect("[ERROR] Failed to execute game!");
            },
            GameType::Wine => {
                Command::new("wine")
                    .arg("$PREFIX/drive_c/Program\\ Files")
                    .arg("-applaunch")
                    .arg(&game.app_id)
                    .stdout(output)
                    .stderr(error)
                    .spawn()
                    .expect("[ERROR] Failed to execute game!");
            }
        }
    }
}
