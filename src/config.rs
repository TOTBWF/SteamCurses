use std::path::Path;

pub struct Config {
    pub steam_path: Box<Path>,
    pub wine_steam_path: Option<Box<Path>>,
    pub log_path: Option<Box<Path>>,
}

impl Config {

    pub fn new(steam_path: &Path) -> Self {
        Config {
            steam_path: Box::from(steam_path),
            wine_steam_path: None,
            log_path: None
        }
    }

    pub fn log_path(mut self, log_path: Option<&Path>) -> Self {
        self.log_path = log_path.map(Box::from);
        self
    }

    pub fn wine_steam_path(mut self, wine_steam_path: Option<&Path>) -> Self {
        self.wine_steam_path = wine_steam_path.map(Box::from);
        self
    }
}
