pub mod vdf_parser;

use std::collections::HashMap;
use std::ops::Index;

#[derive(Debug)]
#[allow(dead_code)]
pub enum VDFValue {
    Str(String),
    Int(i32),
    Float(f32),
    Ptr(u64),
    WStr(String),
    Color(),
    Object(HashMap<String, VDFValue>)
}

#[allow(dead_code)]
impl VDFValue {
    pub fn get(&self, key: &str) -> Option<&VDFValue> {
        match self {
            VDFValue::Object(ref o) => o.get(key),
            _ => None
        }
    }

    // Selects a sub-object, consuming the larger structure
    pub fn select(self, key: &str) -> Option<VDFValue> {
        match self {
            VDFValue::Object(mut o) => o.remove(key),
            _ => None
        }
    }

    pub fn as_string(&self) -> Option<&str> {
        match self {
            VDFValue::Str(s) => Some(s),
            _ => None
        }
    }

    pub fn to_string(self) -> Option<String> {
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

    pub fn to_object(self) -> Option<HashMap<String, VDFValue>> {
        match self {
            VDFValue::Object(o) => Some(o),
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
