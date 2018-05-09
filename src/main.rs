#[macro_use]
extern crate nom;
extern crate ncurses;
mod acf_parser;
mod acf;

use std::fs;
use std::fs::File;
use std::io::prelude::*;
use std::path::Path;

use ncurses::*;
use acf_parser::{parse_acf};
use acf::*;


fn init_window() -> WINDOW {
    initscr();

    cbreak();
    noecho();

    keypad(stdscr(), true);


    let win = newwin(LINES() - 3, COLS() - 1, 0, 0);

    keypad(win, true);

    return win;
}

fn print_title(win: WINDOW, title: &str) {
    let len = title.len() as i32;
    let mut x = 0;
    let mut y = 0;
    getmaxyx(win, &mut y, &mut x);
    mvwprintw(win, 0, x/2 - len/2, title);
    refresh();
}

fn init_menu(win: WINDOW, items: &mut Vec<ITEM>) -> MENU {

    let menu = new_menu(items);
    set_menu_format(menu, LINES() - 4, 0);
    set_menu_win(menu, win);
    set_menu_sub(menu, derwin(win, LINES() - 4, COLS() - 2, 1, 0));

    mvprintw(LINES() - 1, 0, "q to exit");
    print_title(win, "SteamCurses");

    post_menu(menu);
    wrefresh(win);
    return menu;
}

fn init_items(games: Vec<AcfValue>) -> Vec<ITEM> {
    let mut items: Vec<ITEM> = Vec::with_capacity(games.len());
    for game in games {
        match game["AppState"]["name"] {
            AcfValue::Str(ref name) => items.push(new_item(name.clone(), String::from(""))),
            _ => continue
        }
    }
    items
}

fn print_menu(win: WINDOW, menu: MENU) {
    let mut ch = wgetch(win);
    while ch != 'q' as i32 {
        match ch {
            KEY_UP => {
                menu_driver(menu, REQ_UP_ITEM);
            },
            KEY_DOWN => {
                menu_driver(menu, REQ_DOWN_ITEM);
            }
            _ => {}
        }
        wrefresh(win);
        ch = wgetch(win);
    }
}

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
    let win = init_window();
    let games = parse_manifests(Path::new("/home/reed/.steam/steam/steamapps/"));
    let mut items = init_items(games);
    let menu = init_menu(win, &mut items);
    print_menu(win, menu);

    unpost_menu(menu);
    endwin();
}
