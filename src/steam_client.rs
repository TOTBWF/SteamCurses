use std::process::*;
use std::env;
use std::fs::{File, OpenOptions};
use std::io::Write;

use config::*;

pub struct SteamClient {
    proc: Child,
    log_file: Option<File>
}

impl Drop for SteamClient {
    fn drop(&mut self) {
        self.proc.kill();
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
                .open(p)
                .expect("[Error] Cannot open log file!")
        });
        let output = mk_pipe(&log_file);
        let error = mk_pipe(&log_file);

        // let mut command = Command::new("steam")
        //     .stdout(output)
        //     .stderr(error)
        // if !config.steam_visible {
        //     command.arg("-silent")
        //     .env("LD_PRELOAD", "/usr/lib32/steam_injector.so");
        // }
        let mut args = vec!["-silent"];
        let mut env = Vec::new();
        if !config.steam_visible {
            env.push(("LD_PRELOAD", "/usr/lib32/steam_injector.so"));
        }

        let proc = Command::new("steam")
            .args(args)
            .envs(env)
            .stdout(output)
            .stderr(error)
            .spawn()
            .expect("[ERROR] Failed to execute steam!");
        SteamClient { proc: proc, log_file: log_file }
    }

    pub fn launch_game(&mut self, app_id: &str) {
        let output = mk_pipe(&self.log_file);
        let error = mk_pipe(&self.log_file);
        Command::new("steam")
            .arg("-applaunch")
            .arg(app_id)
            .stdout(output)
            .stderr(error)
            .spawn()
            .expect("[ERROR] Failed to execute game!");
    }
}

impl Drop for SteamClient {
    fn drop(self) -> () {
        self.proc.kill();
    }
}
