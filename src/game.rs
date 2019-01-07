use std::cmp::Ordering;
use vdf::*;

#[derive(PartialEq, Eq)]
pub enum GameType {
    Native,
    Wine
}

#[derive(PartialEq, Eq)]
pub struct Game {
    pub name: String,
    pub app_id: String,
    pub game_type: GameType
}

impl Game {
    pub fn new(vdf: VDFValue, game_type: GameType) -> Game {
        let name = vdf["AppState"]["name"].as_string().expect("Missing name field");
        let app_id = vdf["AppState"]["appid"].as_string().expect("Missing appid field");
        Game {
            name: name.to_string(),
            app_id: app_id.to_string(),
            game_type: game_type
        }
    }
}

impl AsRef<str> for Game {
    fn as_ref(&self) -> &str {
        &self.name
    }
}

impl PartialOrd for Game {
    fn partial_cmp(&self, other: &Game) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for Game {
    fn cmp(&self, other: &Game) -> Ordering {
        self.name.cmp(&other.name)
    }
}
