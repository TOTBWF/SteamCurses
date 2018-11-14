use std::io;

use std::sync::mpsc;
use std::thread;

use termion::event;
use termion::input::TermRead;

use tui::Terminal;
use tui::backend::RawBackend;
use tui::style::{Color, Style};
use tui::widgets::{Paragraph, SelectableList, Widget};
use tui::layout::{Group, Size, Direction, Rect};

use steam_client::*;
use game::*;

enum Mode {
    Command,
    Select,
    Alert
}

enum Event {
    Input(event::Key),
    Quit
}

struct App<'a> {
    games: &'a Vec<Game>,
    client: &'a mut SteamClient,
    selected: usize,
    command: String,
    command_tx: &'a mpsc::Sender<Event>,
    mode: Mode
}

pub fn render_ui(games: &Vec<Game>, client: &mut SteamClient) {
    let backend = RawBackend::new().unwrap();
    let mut terminal = Terminal::new(backend).unwrap();


    // Perform keyboard input on a separate thread
    let (tx, rx) = mpsc::channel();

    let mut app = App { 
        games: games, 
        selected: 0, 
        client: client,
        command: String::from(""),
        command_tx: &tx.clone(),
        mode: Mode::Select,
    };

    thread::spawn(move || {
        let input_tx = tx.clone();
        let stdin = io::stdin();
        for c in stdin.keys() {
            let evt = c.unwrap();
            input_tx.send(Event::Input(evt)).unwrap();
        }
    });

    terminal.clear().unwrap();
    terminal.hide_cursor().unwrap();

    loop {
        draw(&app, &mut terminal).unwrap();
        let evt = rx.recv().unwrap();
        match evt {
            Event::Input(key) => {
                match app.mode {
                    Mode::Select => handle_event_select(&mut app, key),
                    Mode::Command => handle_event_command(&mut app, key),
                    Mode::Alert => handle_event_alert(&mut app, key)
                }
            }
            Event::Quit => {
                break;
            }
        }
    }

    terminal.show_cursor().unwrap();
    terminal.clear().unwrap();
}

fn draw(app: &App, t: &mut Terminal<RawBackend>) -> Result<(), io::Error> {
    let size = t.size()?;
    Group::default()
        .direction(Direction::Vertical)
        .sizes(&[Size::Fixed(size.height - 1), Size::Fixed(1)])
        .render(t, &size, |t, chunks| {
            SelectableList::default()
                .items(app.games)
                .select(app.selected)
                .highlight_symbol("-")
                .render(t, &chunks[0]);
            draw_command_bar(app, t, &chunks[1])
        });
    t.draw()
}

fn draw_command_bar(app: &App, t: &mut Terminal<RawBackend>, area: &Rect) {
    let text = match app.mode {
        Mode::Command => String::from(":") + &app.command,
        Mode::Select => String::from("-- SELECT --"),
        Mode::Alert => String::from("") + &app.command
    };
    Paragraph::default()
        .wrap(false)
        .text(&text)
        .style(Style::default().bg(Color::DarkGray))
        .render(t, area);
}

// Used to handle events when in select mode
fn handle_event_select(app: &mut App, evt: event::Key) {
    match evt {
        event::Key::Char(':') => {
            app.mode = Mode::Command;
        }
        event::Key::Up => {
            if app.selected > 0 {
                app.selected -= 1
            };
        }
        event::Key::Down => {
            if app.selected < app.games.len() - 1 {
                app.selected += 1
            };
        }
        event::Key::Char('\n') => {
            let game = &app.games[app.selected];
            app.command = format!("Launching {}...", &game.name);
            app.mode = Mode::Alert;
            app.client.launch_game(&game.app_id);
        }
        event::Key::Ctrl('c') => {
            app.command = "Type :q and press <Enter> to exit steamcurses".to_string();
            app.mode = Mode::Alert;
        }
        _ => {}
    }
}

fn handle_event_command(app: &mut App, evt: event::Key) {
    match evt {
        event::Key::Char('\n') => {
            execute_command(&app.command, &app.command_tx);
            app.command = String::from("");
            app.mode = Mode::Select;
        }
        event::Key::Char(c) => {
            app.command.push(c);
        }
        event::Key::Esc => {
            app.command = String::from("");
            app.mode = Mode::Select;
        }
        event::Key::Backspace => {
            let l = app.command.len() - 1;
            app.command.truncate(l);
        }
        event::Key::Ctrl('c') => {
            app.command = String::from("Type :q and press <Enter> to exit steamcurses");
            app.mode = Mode::Alert;
        }
        _ => {}
    }
}

fn handle_event_alert(app: &mut App, evt: event::Key) {
    match evt {
        event::Key::Char('\n') => {
            app.command = String::from("");
            app.mode = Mode::Select;
        }
        event::Key::Esc => {
            app.command = String::from("");
            app.mode = Mode::Select;
        }
        event::Key::Char(':') => {
            app.command = String::from("");
            app.mode = Mode::Command;
        }
        _ => {}
    }
}

fn execute_command(command: &str, tx: &mpsc::Sender<Event>) {
    match command {
        "q" => tx.send(Event::Quit).unwrap(),
        _ => {}
    }
}
