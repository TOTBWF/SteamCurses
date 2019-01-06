use vdf::*;

pub enum GameType {
    Native,
    Wine
}

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
        // &format!("{} ({})", self.name, self.app_id)
        // &(self.name + " (" + self.app_id + ")")
    }
}
