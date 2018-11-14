use std::path::Path;

use vdf::*;

pub struct Config<'a> {
    pub username: &'a str,
    pub steam_path: Option<&'a Path>,
    pub wine_steam_path: Option<&'a Path>,
    pub log_path: Option<&'a Path>,
    pub steam_visible: bool
}

impl <'a> Config<'a> {
    pub fn from_vdf(v: &'a VDFValue) -> Config<'a> {
        let c = v.get("config")
            .and_then(|c| c.as_object())
            .expect("[ERROR] Missing \"config\" object!");
        Config {
            username: c.get("username")
                .and_then(|u| u.as_string())
                .expect("[ERROR] Config missing username!"),
            steam_path: c.get("steam_path")
                .and_then(|p| p.as_string())
                .map(|p| Path::new(p)),
            wine_steam_path: c.get("wine_steam_path")
                .and_then(|p| p.as_string())
                .map(|p| Path::new(p)),
            log_path: None,
            steam_visible: false
        }
    }

    pub fn steam_visible(mut self, f: bool) -> Self {
        self.steam_visible = f;
        self
    }

    pub fn log_path(mut self, log_path: Option<&'a Path>) -> Self {
        self.log_path = log_path;
        // self.log_path = log_path;
        self
    }
}
