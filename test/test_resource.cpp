#define BOOST_TEST_MODULE RVN_SQLITE_RESOURCE
#include <boost/test/unit_test.hpp>

#include <resource_database.h>

#include "test_helpers.h"

using MD = reven::sqlite::Metadata;
using RDb = reven::sqlite::ResourceDatabase;

// Allows to write MD
class TestMDWriter : reven::sqlite::MetadataWriter {
public:
	static constexpr std::uint32_t type = 42;
	static constexpr char format_version[] = "1.0.0-dummy";
	static constexpr char tool_name[] = "TestMetaDataWriter";
	static constexpr char tool_version[] = "1.0.0";
	static constexpr char tool_info[] = "Tests version 1.0.0";
	static constexpr std::uint64_t generation_date = 42424242;

	static MD dummy_md() {
		return write(type, format_version, tool_name, tool_version, tool_info, generation_date);
	}

	static MD dummy_md_2() {
		return write(type, format_version, tool_name, tool_version, tool_info, generation_date + 1);
	}
};

bool operator==(const MD& left, const MD& right) {
	return left.generation_date() == right.generation_date() and
	        left.format_version() == right.format_version() and
	        left.tool_info() == right.tool_info() and
	        left.tool_version() == right.tool_version() and
	        left.tool_name() == right.tool_name() and
	        left.type() == right.type();
}

bool operator!=(const MD& left, const MD& right) { return not (left == right); }

constexpr std::uint32_t TestMDWriter::type;
constexpr char TestMDWriter::format_version[];
constexpr char TestMDWriter::tool_name[];
constexpr char TestMDWriter::tool_version[];
constexpr char TestMDWriter::tool_info[];
constexpr std::uint64_t TestMDWriter::generation_date;

// A db that only contains the MD
BOOST_AUTO_TEST_CASE(test_empty_db)
{
	auto rdb = RDb::from_memory(TestMDWriter::dummy_md());

	BOOST_CHECK(rdb.metadata() == TestMDWriter::dummy_md());
	BOOST_CHECK_EQUAL(rdb.metadata().type(), TestMDWriter::type);
	BOOST_CHECK_EQUAL(rdb.metadata().format_version(), TestMDWriter::format_version);
	BOOST_CHECK_EQUAL(rdb.metadata().tool_name(), TestMDWriter::tool_name);
	BOOST_CHECK_EQUAL(rdb.metadata().tool_version(), TestMDWriter::tool_version);
	BOOST_CHECK_EQUAL(rdb.metadata().tool_info(), TestMDWriter::tool_info);
	BOOST_CHECK_EQUAL(rdb.metadata().generation_date(), TestMDWriter::generation_date);
}

// Try to open a DB without MD as if it had MD
BOOST_AUTO_TEST_CASE(test_no_md_db)
{
	auto db = Db::from_memory();
	BOOST_CHECK_THROW(RDb::from_db(std::move(db)), reven::sqlite::ReadMetadataError);
}

// Convert a DB without MD to a DB with MD
BOOST_AUTO_TEST_CASE(test_some_data)
{
	auto db = create_test_table();
	auto rdb = RDb::convert_db(std::move(db), TestMDWriter::dummy_md());
	auto insert_stmt = get_insert_stmt(rdb);
	constexpr std::uint64_t x_insert = 42;
	insert_stmt.bind_arg_cast(1, x_insert, "x");
	insert_stmt.step();

	auto fetch_stmt = get_fetch_stmt(rdb);
	BOOST_CHECK(fetch_stmt.step() == Stmt::StepResult::Row);
	std::uint64_t x_fetch = fetch_stmt.column_u64(0);
	BOOST_CHECK_EQUAL(x_fetch, x_insert);

	BOOST_CHECK(rdb.metadata() == TestMDWriter::dummy_md());
}

// Open twice the same RDb
BOOST_AUTO_TEST_CASE(test_open_reopen)
{
	auto rdb = RDb::from_memory(TestMDWriter::dummy_md());
	MD md = rdb.metadata();
	// forces re-reading md
	auto rdb_2 = RDb::from_db(std::move(rdb));
	BOOST_CHECK(md == rdb_2.metadata());
}

BOOST_AUTO_TEST_CASE(test_set_metadata)
{
	auto md = TestMDWriter::dummy_md();
	auto rdb = RDb::from_memory(md);
	BOOST_CHECK(rdb.metadata() == md);

	auto md_2 = TestMDWriter::dummy_md_2();
	rdb.set_metadata(md_2);
	BOOST_CHECK(md_2 == rdb.metadata());
	// forces re-reading md
	auto rdb_2 = RDb::from_db(std::move(rdb));
	BOOST_CHECK(md_2 == rdb_2.metadata());
	BOOST_CHECK(md != rdb_2.metadata());
}

// Convert twice the same RDb
BOOST_AUTO_TEST_CASE(test_convert_twice)
{
	auto db = Db::from_memory();
	auto rdb = RDb::convert_db(std::move(db), TestMDWriter::dummy_md());
	BOOST_CHECK_THROW(RDb::convert_db(std::move(rdb), TestMDWriter::dummy_md()), reven::sqlite::WriteMetadataError);
}
