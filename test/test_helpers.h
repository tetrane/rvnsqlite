#include <sqlite.h>

using Db = reven::sqlite::Database;
using Stmt = reven::sqlite::Statement;

inline Db create_test_table() {
	auto db = Db::from_memory();
	db.exec("Create table test (x int8);", "Could not create 'test' table");
	return db;
}

inline Stmt get_insert_stmt(Db& db) {
	return Stmt(db, "insert into test values (?);");
}

inline Stmt get_fetch_stmt(Db& db) {
	return Stmt(db, "select x from test;");
}

