#[macro_use]
extern crate nom;
extern crate tui;
extern crate termion;
extern crate clap;

mod vdf;
mod game;
mod config;
mod steam_client;
mod user_interface;

use std::fs;
use std::fs::{File, OpenOptions};
use std::io::prelude::*;
use std::io;
use std::env;
use std::path::Path;
use clap::{Arg, App};

use config::*;
use game::*;
use vdf::*;
use steam_client::*;
use user_interface::*;


fn parse_manifest(path: &Path) -> VDFValue {
    let mut contents = String::new();
    let mut file = File::open(path).expect("Error reading file!");
    file.read_to_string(&mut contents);
    vdf_parser::parse_vdf(&contents)
}

fn parse_manifests(steam_path: &Path) -> Vec<VDFValue> {
    let mut manifests = Vec::new();
    let files = fs::read_dir(steam_path).unwrap();
    for file in files {
        let f = file.unwrap();
        let fname = f.file_name();
        let fname_str = fname.to_str().unwrap();
        if fname_str.contains("appmanifest") {
            manifests.push(parse_manifest(&f.path()));
        }
    }
    manifests
}

fn load_config(config_path: Option<&str>) -> vdf::VDFValue {
    match config_path {
        Some(ref p) => parse_manifest(Path::new(p)),
        None => {
            let home_var = env::var("HOME")
                .expect("[ERROR] Could not determine home directory!");
            let xdg_config_var = env::var("XDG_CONFIG_HOME");
            let config_dir_path = xdg_config_var
                .map(|v| Path::new(&v).join("steamcurses"))
                .unwrap_or(Path::new(&home_var).join(".config").join("steamcurses"));

            if !config_dir_path.exists() {
                fs::create_dir(&config_dir_path)
                    .expect("[ERROR] Could not create config directory!");
            }

            let config_file_path = config_dir_path.join("steamcurses.conf");

            if !config_file_path.exists() {
                println!("Creating config file {:?}", config_file_path);

                print!("Enter your steam username: ");
                io::stdout().flush();
                let mut username = String::new();
                io::stdin().read_line(&mut username).unwrap();

                let config_contents = format!("\"config\" {{\"username\" \"{}\"}}", &username[.. username.len() - 1]);

                File::create(&config_file_path)
                    .map(|mut f| f.write_all(&config_contents.as_bytes()).unwrap())
                    .expect("[ERROR] Could not create config file!");
            }
            parse_manifest(&config_file_path)
        }
    }
}

fn main() {
    let matches = App::new("steamcurses")
        .version("0.2")
        .author("Reed Mullanix")
        .about("A terminal steam client")
        .arg(Arg::with_name("logfile")
             .short("l")
             .long("logfile")
             .value_name("FILE")
             .help("Sets a custom log file.")
             .takes_value(true))
        .arg(Arg::with_name("config")
             .short("c")
             .long("config")
             .value_name("FILE")
             .help("Sets a custom config file. Defaults to $XDG_CONFIG_HOME/steamcurses/steamcurses.conf")
             .takes_value(true))
        .arg(Arg::with_name("steam_prompt")
             .short("p")
             .long("prompt")
             .help("Uses the steam login prompt")
        )
        .get_matches();

    let config_vdf = load_config(matches.value_of("config"));
    let config = Config::from_vdf(&config_vdf)
        .log_path(matches.value_of("logfile").map(|l| Path::new(l)))
        .steam_visible(matches.is_present("steam_prompt"));
    let mut steam_client = SteamClient::new(config);
    let manifests = parse_manifests(Path::new("/home/reed/.steam/steam/steamapps/"));
    let games: Vec<Game> = manifests.into_iter().map(Game::new).collect();
    render_ui(&games, &mut steam_client)
}
