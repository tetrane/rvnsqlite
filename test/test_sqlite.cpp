#define BOOST_TEST_MODULE RVN_SQLITE
#include <boost/test/unit_test.hpp>

#include <sqlite.h>

#include <sqlite3.h>

#include "test_helpers.h"

// create empty database in memory
BOOST_AUTO_TEST_CASE(test_empty_db)
{
	Db::from_memory();
}

// Test simple insertion
BOOST_AUTO_TEST_CASE(test_insert_db)
{
	// db creation
	auto db = create_test_table();

	// insert value
	auto statement = get_insert_stmt(db);
	statement.bind_arg(1, static_cast<std::int32_t>(0), "x");
	BOOST_CHECK(statement.step() == Stmt::StepResult::Done);
}

// Test retrieving previously inserted value
BOOST_AUTO_TEST_CASE(test_retrieve_db)
{
	// the value to insert
	constexpr std::uint64_t val = 42;

	// db creation
	auto db = create_test_table();

	// insert value
	auto statement = get_insert_stmt(db);
	statement.bind_arg_cast(1, val, "x");
	BOOST_CHECK(statement.step() == Stmt::StepResult::Done);

	// retrieve value
	statement = get_fetch_stmt(db);
	BOOST_CHECK(statement.step() == Stmt::StepResult::Row);
	BOOST_CHECK(statement.column_type(0) == Stmt::Type::Integer);
	BOOST_CHECK(statement.column_i64(0) == val);
}

// Test slide insertion and extraction functions
BOOST_AUTO_TEST_CASE(test_insert_u64_to_i64)
{
	// the values to insert
	constexpr std::uint64_t val = 0;
	constexpr std::uint64_t val2 = std::numeric_limits<std::uint64_t>::max();
	constexpr std::uint64_t val3 = std::numeric_limits<std::int64_t>::max();

	// db creation
	auto db = create_test_table();

	auto statement = get_insert_stmt(db);
	// insert 0
	statement.bind_arg_slide(1, val, "x");
	BOOST_CHECK(statement.step() == Stmt::StepResult::Done);
	statement.reset();
	// insert uint64_t max
	statement.bind_arg_slide(1, val2, "x");
	BOOST_CHECK(statement.step() == Stmt::StepResult::Done);
	statement.reset();
	// insert int64_t max
	statement.bind_arg_slide(1, val3, "x");
	BOOST_CHECK(statement.step() == Stmt::StepResult::Done);

	statement = get_fetch_stmt(db);
	// retrieve 0
	BOOST_CHECK(statement.step() == Stmt::StepResult::Row);
	BOOST_CHECK(statement.column_type(0) == Stmt::Type::Integer);
	BOOST_CHECK(statement.column_u64_slide(0) == val);
	// retrieve uint64_t max
	BOOST_CHECK(statement.step() == Stmt::StepResult::Row);
	BOOST_CHECK(statement.column_u64_slide(0) == val2);
	// retrieve int64_t max
	BOOST_CHECK(statement.step() == Stmt::StepResult::Row);
	BOOST_CHECK(statement.column_u64_slide(0) == val3);
}

BOOST_AUTO_TEST_CASE(test_retrieve_empty_db)
{
	// db creation
	auto db = create_test_table();

	// retrieve stmt
	auto statement = get_fetch_stmt(db);
	BOOST_CHECK(statement.step() == Stmt::StepResult::Done);
}

BOOST_AUTO_TEST_CASE(test_create_from_raw)
{
	sqlite3* raw_db = nullptr;
	if (sqlite3_open_v2(":memory:", &raw_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr)) {
		throw std::runtime_error("Cannot create DB in memory");
	}
	Db{raw_db};
}
