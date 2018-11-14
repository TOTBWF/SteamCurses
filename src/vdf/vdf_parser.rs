use std::collections::HashMap;
use std::hash::Hash;
use std::str;

use vdf::{VDFValue};



named!(
    string_content<String>,
    map!(escaped_transform!(
            take_until_either!("\"\\"),
            '\\',
            alt!(
                char!('\\') => { |_| &b"\\"[..] } |
                char!('\"') => { |_| &b"\""[..] } |
                char!('n') => { |_| &b"\n"[..] } |
                char!('r') => { |_| &b"\r"[..] } |
                char!('t') => { |_| &b"\t"[..] }
            )
        ),
        |s| String::from_utf8_lossy(&s).into_owned()
    )
);

named!(
    string<String>,
    delimited!( char!('\"'), string_content, char!('\"'))
);

named!(
    key_value<(String, VDFValue)>,
    ws!(pair!(string, vdf_value))
);

named!(
    object<HashMap<String, VDFValue>>,
    ws!(map!(
        delimited!(
            char!('{'),
            many0!(key_value),
            char!('}')
        ),
        |tuple_vec| tuple_vec
            .into_iter()
            .collect()
    ))
);

named!(
    vdf_value<VDFValue>,
    dbg!(ws!(alt!(
        object => { |o| VDFValue::Object(o) } |
        string => { |s| VDFValue::Str(s) }
    )))
);

fn singleton<K: Eq + Hash, V>(k: K, v: V) -> HashMap<K,V> {
    let mut m = HashMap::with_capacity(1);
    m.insert(k, v);
    return m;
}

named!(
    vdf<VDFValue>,
    dbg!(ws!(map!(
        pair!(string, object),
        |(k,v)| VDFValue::Object(singleton(k, VDFValue::Object(v)))
    )))
);


pub fn parse_vdf(s: &str) -> VDFValue {
    vdf(s.as_bytes()) .to_result().unwrap()
}

/*
 * The binary VDF format begins with a header. The 1st 4 bytes act as a way to determine the file type.
 * The 2 possible options are:
 * 
 * 0x00 0x01 0x02 0x03
 * -------------------
 * 0x27 0x44 0x56 0x__ (appinfo.vdf)
 * 0x27 0x55 0x56 0x__ (packageinfo.vdf)
 * 
 */ 

// named!(bvdf_header<()>,
    
// );
