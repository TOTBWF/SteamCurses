use ncurses::*;

use launch::*;
use acf::*;

pub struct UI {
    window: WINDOW,
    status_bar: WINDOW,
    menu: MENU
}

impl Drop for UI {
    fn drop(&mut self) {
        unpost_menu(self.menu);
        endwin();
    }
}

fn update_status(status_bar: WINDOW, status: &str) {
    mvwprintw(status_bar, 1, 1, status);
    wrefresh(status_bar);
}

fn print_title(win: WINDOW, title: &str) {
    let len = title.len() as i32;
    let mut x = 0;
    let mut y = 0;
    getmaxyx(win, &mut y, &mut x);
    mvwprintw(win, 0, x/2 - len/2, title);
}


fn init_window() -> WINDOW {
    let win = newwin(LINES(), COLS(), 0, 0);
    keypad(win, true);
    // print_title(win, "SteamCurses");
    return win;
}

fn init_items(games: &Vec<AcfValue>) -> Vec<ITEM> {
    let mut items: Vec<ITEM> = Vec::with_capacity(games.len());
    for game in games {
        match game["AppState"]["name"].value() {
            Some(name) => items.push(new_item(name.clone(), "")),
            None => continue
        }
    }
    items
}

fn init_menu(win: WINDOW, items: &mut Vec<ITEM>) -> MENU {

    let menu = new_menu(items);
    let menu_win = derwin(win, LINES() - 1, COLS() - 2, 1, 0);
    set_menu_format(menu, LINES() - 1, 0);
    set_menu_win(menu, win);
    set_menu_sub(menu, menu_win);

    post_menu(menu);
    wrefresh(win);
    return menu;
}

fn init_status_bar(win: WINDOW) -> WINDOW {
    print_title(win, &format!("Lines: {}", LINES()));
    let status_bar = derwin(win, 3, COLS()- 1, LINES() - 3, 0);
    box_(status_bar, 0, 0);
    status_bar
}

pub fn init_ui(games: &Vec<AcfValue>) -> UI {
    initscr();

    cbreak();
    noecho();

    keypad(stdscr(), true);

    let win = init_window();
    let mut items = init_items(games);
    let menu = init_menu(win, &mut items);
    let status_bar = init_status_bar(win);
    update_status(status_bar, "Test");
    UI { window: win, status_bar: status_bar, menu: menu }
}

pub fn handle_updates(ui: UI, games: &Vec<AcfValue>) {
    let mut ch = wgetch(ui.window);
    while ch != 'q' as i32 {
        match ch {
            KEY_UP => {
                menu_driver(ui.menu, REQ_UP_ITEM);
            },
            KEY_DOWN => {
                menu_driver(ui.menu, REQ_DOWN_ITEM);
            }
            10 => {
                let index = item_index(current_item(ui.menu)) as usize;
                let game = &games[index];
                let app_id = game["AppState"]["appid"].value().unwrap();
                update_status(ui.status_bar, &format!("Launching: {}", app_id));
                let mut game_proc = launch_game(app_id);
                // let res = game_proc.wait().unwrap();
                // update_status(ui.status_bar, &format!("Result: {}", res));
            }
            _ => {}
        }
        ch = wgetch(ui.window);
    }
}