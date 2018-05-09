use std::collections::HashMap;
use std::ops::Index;

#[derive(Debug)]
pub enum AcfValue {
    Str(String),
    Object(HashMap<String, AcfValue>)
}

impl AcfValue {
    pub fn get(&self, key: &str) -> Option<&AcfValue> {
        match self {
            AcfValue::Str(_) => None,
            AcfValue::Object(ref o) => o.get(key)
        }
    }

    pub fn value(&self) -> Option<&str> {
        match self {
            AcfValue::Str(s) => Some(s),
            _ => None
        }
    }
}

impl<'a> Index<&'a str> for AcfValue {
    type Output = AcfValue;
    fn index(&self, key: &str) -> &AcfValue {
        self.get(key).expect("No entry found for key")
    }

}