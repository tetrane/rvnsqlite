#define BOOST_TEST_MODULE RVN_SQLITE_QUERY
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <sqlite.h>
#include <query.h>

#include "test_helpers.h"

namespace {
std::uint64_t fetch_value(Stmt& stmt) {
	return stmt.column_i64(0);
}
} // anonymous namespace

using QU64 = reven::sqlite::Query<std::uint64_t, std::function<std::uint64_t(Stmt&)>>;

// Check that an iterator on an empty statement returns 0 result
BOOST_AUTO_TEST_CASE(test_iter_empty)
{
	// db creation
	auto db = create_test_table();

	// query creation
	auto query = QU64(get_fetch_stmt(db), fetch_value);
	BOOST_CHECK(query.finished());
	BOOST_CHECK(query.current() == query.end());
}

// Check that an iterator on a statement with a single result returns 1 result
BOOST_AUTO_TEST_CASE(test_iter_single)
{
	// the value to insert
	constexpr std::uint64_t val = 42;

	// db creation
	auto db = create_test_table();

	// insert value
	auto statement = get_insert_stmt(db);
	statement.bind_arg_cast(1, val, "x");
	BOOST_CHECK(statement.step() == Stmt::StepResult::Done);

	// query creation
	auto query = QU64(get_fetch_stmt(db), fetch_value);
	BOOST_CHECK(not query.finished());
	BOOST_CHECK(*query.begin() == val);
	++query.begin();
	BOOST_CHECK(query.finished());
	BOOST_CHECK(query.current() == query.end());
}

// Check that an iterator on a statement with two results returns 2 results
BOOST_AUTO_TEST_CASE(test_iter_double)
{
	// the values to insert
	constexpr std::uint64_t x = 42;
	constexpr std::uint64_t y = x / 2;

	// db creation
	auto db = create_test_table();

	// insert values
	auto statement = get_insert_stmt(db);
	statement.bind_arg_cast(1, x, "x");
	BOOST_CHECK(statement.step() == Stmt::StepResult::Done);
	statement.reset();
	statement.bind_arg_cast(1, y, "y");
	BOOST_CHECK(statement.step() == Stmt::StepResult::Done);

	// query creation
	auto query = QU64(get_fetch_stmt(db), fetch_value);
	BOOST_CHECK(not query.finished());
	BOOST_CHECK(*query.begin() == x);
	++query.begin();

	BOOST_CHECK(not query.finished());
	BOOST_CHECK(*query.begin() == y);
	++query.begin();

	BOOST_CHECK(query.finished());
	BOOST_CHECK(query.current() == query.end());
}

// Check that the iterator can be collected into a vector, checking that it is recognized as an InputIterator
BOOST_AUTO_TEST_CASE(test_iter_vec)
{
	// the values to insert
	constexpr std::uint64_t x = 42;
	constexpr std::uint64_t y = x / 2;

	// db creation
	auto db = create_test_table();

	// insert values
	auto statement = get_insert_stmt(db);
	statement.bind_arg_cast(1, x, "x");
	BOOST_CHECK(statement.step() == Stmt::StepResult::Done);
	statement.reset();
	statement.bind_arg_cast(1, y, "y");
	BOOST_CHECK(statement.step() == Stmt::StepResult::Done);

	// query creation
	auto query = QU64(get_fetch_stmt(db), fetch_value);
	std::vector<std::uint64_t> vec(query.begin(), query.end());

	BOOST_CHECK(vec == std::vector<std::uint64_t>({42, 21}));
}

BOOST_AUTO_TEST_CASE(test_fail_bind)
{
	// db creation
	auto db = create_test_table();

	// insert values
	auto statement = get_insert_stmt(db);
	try {
		statement.bind_arg_cast(4, static_cast<std::uint64_t>(42), "foo");
	} catch(const reven::sqlite::DatabaseError& e) {
		BOOST_CHECK_EQUAL(e.what(), "Can't bind foo: bind or column index out of range");
	}
}
