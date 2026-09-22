#[cfg(feature = "csource")]
#[test]
fn test_c_core_integration() {
    use hyle::csource::{self, FieldDef, FieldType};
    use hyle::{Query, Row, Value};
    use indexmap::IndexMap;

    let fields = [
        FieldDef { name: "title", kind: FieldType::String },
        FieldDef { name: "year", kind: FieldType::Int },
    ];
    csource::register_source("rust_c_test", &fields);

    let mut r1: Row = IndexMap::new();
    r1.insert("id".to_string(), Value::String("1".to_string()));
    r1.insert("title".to_string(), Value::String("Song One".to_string()));
    r1.insert("year".to_string(), Value::Int(1990));
    csource::source_put("rust_c_test", &r1);

    let mut r2: Row = IndexMap::new();
    r2.insert("id".to_string(), Value::String("2".to_string()));
    r2.insert("title".to_string(), Value::String("Song Two".to_string()));
    r2.insert("year".to_string(), Value::Int(2000));
    csource::source_put("rust_c_test", &r2);

    let q = Query::default();
    let (rows, total) = csource::source_query("rust_c_test", &q).expect("query failed");
    assert_eq!(total, 2);
    assert_eq!(rows.len(), 2);

    csource::source_del("rust_c_test", "1");
    let (rows, total) = csource::source_query("rust_c_test", &q).expect("query after del failed");
    assert_eq!(total, 1);
    assert_eq!(rows[0].get("id"), Some(&Value::String("2".to_string())));
}
