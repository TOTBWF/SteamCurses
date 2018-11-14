pub mod vdf_parser;

use std::collections::HashMap;
use std::ops::Index;

#[derive(Debug)]
pub enum VDFValue {
    Str(String),
    Int(i32),
    Float(f32),
    Ptr(u64),
    WStr(String),
    Color(),
    Object(HashMap<String, VDFValue>)
}

impl VDFValue {
    pub fn get(&self, key: &str) -> Option<&VDFValue> {
        match self {
            VDFValue::Object(ref o) => o.get(key),
            _ => None
        }
    }

    pub fn as_string(&self) -> Option<&str> {
        match self {
            VDFValue::Str(s) => Some(s),
            _ => None
        }
    }

    pub fn as_object(&self) -> Option<&HashMap<String, VDFValue>> {
        match self {
            VDFValue::Object(ref o) => Some(o),
            _ => None
        }
    }

}

impl<'a> Index<&'a str> for VDFValue {
    type Output = VDFValue;
    fn index(&self, key: &str) -> &VDFValue {
        self.get(key).expect("No entry found for key")
    }

}
