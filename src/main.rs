#[macro_use]
extern crate nom;
extern crate ncurses;
mod acf_parser;
mod acf;
mod launch;
mod tui;

use std::fs;
use std::fs::File;
use std::io::prelude::*;
use std::path::Path;

use ncurses::*;

use acf_parser::{parse_acf};
use acf::*;
use launch::*;
use tui::*;

fn parse_manifest(path: &Path) -> AcfValue {
    let mut contents = String::new();
    let mut file = File::open(path).expect("Error reading file!");
    file.read_to_string(&mut contents);
    parse_acf(&contents)
}

fn parse_manifests(steam_path: &Path) -> Vec<AcfValue> {
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

fn main() {

    let mut steam_proc = launch_steam();

    let games = parse_manifests(Path::new("/home/reed/.steam/steam/steamapps/"));

    let ui = init_ui(&games);
    handle_updates(ui, &games);

    steam_proc.kill();
}
