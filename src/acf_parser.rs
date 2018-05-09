use std::collections::HashMap;
use std::hash::Hash;
use std::str;

use nom::{alphanumeric};

use acf::{AcfValue};

named!(
    string<&str, &str>,
    delimited!(
        char!('\"'),
        call!(alphanumeric),
        char!('\"')
    )
);

named!(
    key_value<&str, (&str, AcfValue)>,
    ws!(pair!(string, acf_value))
);

named!(
    object<&str, HashMap<String, AcfValue>>,
    ws!(map!(
        delimited!(
            char!('{'),
            many0!(key_value),
            char!('}')
        ),
        |tuple_vec| tuple_vec
            .into_iter()
            .map(|(k, v)| (String::from(k), v))
            .collect()
    ))
);

named!(
    acf_value<&str, AcfValue>,
    ws!(alt!(
        object => { |o| AcfValue::Object(o) } |
        string => { |s| AcfValue::Str(String::from(s)) }
    ))
);

fn singleton<K: Eq + Hash, V>(k: K, v: V) -> HashMap<K,V> {
    let mut m = HashMap::with_capacity(1);
    m.insert(k, v);
    return m;
}

named!(
    acf<&str, AcfValue>,
    ws!(map!(
        pair!(string, object),
        |(k,v)| AcfValue::Object(singleton(String::from(k), AcfValue::Object(v)))
    ))
);

pub fn parse_acf(s: &str) -> AcfValue {
    return acf(s).to_result().unwrap();
}
