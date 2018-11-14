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
use std::path::Path;
use clap::{Arg, App};

use config::*;
use game::*;
use vdf::*;
use steam_client::*;
use user_interface::*;

fn parse_manifest(path: &Path) -> VDFValue {
    let mut contents = String::new();
    let mut file = File::open(path).expect("[ERROR] Cannot open manifest file");
    file.read_to_string(&mut contents).unwrap();
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

fn load_config(custom_config_path: Option<&str>) -> Config {
    let xdg_dirs = xdg::BaseDirectories::with_prefix("steamcurses").unwrap();
    let xdg_config_path = xdg_dirs.place_config_file ("steamcurses.conf")
        .expect("[ERROR] Cannot create config file");

    let config_path = match custom_config_path {
        Some(p) => {
            Path::new(p)
        },
        None => {
            let mut f = OpenOptions::new()
                .create(true)
                .read(true)
                .write(true)
                .open(xdg_config_path.as_path())
                .expect("[ERROR] Cannot create config file");
            f.write_all(b"\"config\" {}\n").expect("[ERROR] Cannot create config file");
            xdg_config_path.as_path()
        }
    };

    let config_dict = parse_manifest(config_path)
        .select("config")
        .and_then(VDFValue::to_object)
        .unwrap();

    let xdg_steam_dirs = xdg::BaseDirectories::with_prefix("Steam").unwrap();
    let xdg_steam_path = xdg_steam_dirs.get_data_home();
    let steam_path = config_dict.get("steam_path")
        .and_then(VDFValue::as_string)
        .map(Path::new)
        .unwrap_or(xdg_steam_path.as_path());
    let wine_steam_path = config_dict.get("wine_steam_path")
        .and_then(VDFValue::as_string)
        .map(Path::new);

    let config = Config::new(steam_path);
    config.wine_steam_path(wine_steam_path)
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
        .get_matches();

    // let steam_path_default = Path::new(home_var)
    //     .join("")

    let config = load_config(matches.value_of("config"));
    let manifests = parse_manifests(&config.steam_path.as_ref().join("steamapps"));
    let mut steam_client = SteamClient::new(config);

    let games: Vec<Game> = manifests.into_iter().map(Game::new).collect();
    render_ui(&games, &mut steam_client)
}
