#include <sqlite.h>

#include <limits>
#include <sqlite3.h>

// #define ACTIVATE_DEBUG_LOGS

#ifdef ACTIVATE_DEBUG_LOGS
#include <iostream>
#define STATEMENT(s) do { s; } while(0)
#define DEBUG_LOG(m) STATEMENT(std::cerr << "Sqlite: " << m << std::endl)
#else
#define DEBUG_LOG(m)
#endif

using namespace std::literals::string_literals;

namespace reven {
namespace sqlite {

namespace {

int from(Database::OpenMode mode) {
	switch (mode) {
	case Database::OpenMode::Create:
		return SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	case Database::OpenMode::ReadOnly:
		return SQLITE_OPEN_READONLY;
	case Database::OpenMode::ReadWrite:
		return SQLITE_OPEN_READWRITE;
	}
	throw std::logic_error("Unreachable! Wrong open mode.");
}

Statement::Type to_type(int sqlite_type) {
	using Type = Statement::Type;
	switch (sqlite_type) {
	case SQLITE_INTEGER:
		return Type::Integer;
	case SQLITE_FLOAT:
		return Type::Float;
	case SQLITE_TEXT:
		return Type::Text;
	case SQLITE_BLOB:
		return Type::Blob;
	case SQLITE_NULL:
		return Type::Null;
	}
	throw std::logic_error("Unknown sqlite type (" + std::to_string(sqlite_type) + "!");
}

const char* to_string(Database::OpenMode mode)
{
	using Mode = Database::OpenMode;
	switch (mode) {
	case Mode::Create:
		return "create";
	case Mode::ReadOnly:
		return "open";
	case Mode::ReadWrite:
		return "open R/W";
	}
	throw std::logic_error(
		"Unknown database open mode (" +
		std::to_string(static_cast<std::underlying_type_t<Database::OpenMode>>(mode)) +
		"!"
	);
}

constexpr std::uint64_t signed_to_unsigned_offset = std::uint64_t{1} + std::numeric_limits<std::int64_t>::max();

std::int64_t unsigned_to_signed_int64(std::uint64_t value)
{
	// Ensure we have the correct implementation-defined result
	static_assert(-1 == ~0, "You need two's complement to harness the power of REVEN.");
	return value - signed_to_unsigned_offset;
}

std::uint64_t signed_to_unsigned_int64(std::int64_t value)
{
	return static_cast<uint64_t>(value) + signed_to_unsigned_offset;
}
} // anonymous namespace

Database::Database(const char* filename, Database::OpenMode mode)
{
	int flags = from(mode);
	sqlite3* raw_db = nullptr;
	if (sqlite3_open_v2(filename, &raw_db, flags, nullptr)) {
		throw DatabaseNotFound("Can't "s + to_string(mode) + " database with filename '" + filename + "'");
	}
	db_ = UniqueDBPtr(raw_db, sqlite3_close);
}

Database::Database(sqlite3* raw_db) : db_(raw_db, sqlite3_close)
{}

void Database::exec(const char* command, const char* error_message)
{
	const auto sqlite_result = sqlite3_exec(db_.get(), command, nullptr, nullptr, nullptr);
	if (sqlite_result) {
		throw DatabaseError(std::string(error_message) + " " + sqlite3_errstr(sqlite_result));
	}
}

std::int64_t Database::last_insert_rowid() const
{
	return sqlite3_last_insert_rowid(db_.get());
}

Statement::Statement(Database& db, const char* stmt_str)
{
	sqlite3_stmt* stmt = nullptr;

	const auto sqlite_result = sqlite3_prepare_v2(db.get(), stmt_str, -1, &stmt, nullptr);
	if (sqlite_result) {
		throw DatabaseError("Can't prepare query statement: "s + sqlite3_errstr(sqlite_result));
	}
	stmt_ = UniqueStmtPtr(stmt, sqlite3_finalize);
}

void Statement::bind_arg_cast(int index, std::uint64_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	const auto sqlite_result = sqlite3_bind_int64(stmt_.get(), index, static_cast<sqlite_int64>(value));
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_arg_throw(int index, std::uint64_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	if (value > static_cast<std::uint64_t>(std::numeric_limits<sqlite_int64>::max())) {
		throw OutOfBoundsError("Value (" + std::to_string(value) + ") in binding is out of bounds");
	}
	const auto sqlite_result = sqlite3_bind_int64(stmt_.get(), index, static_cast<sqlite_int64>(value));
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_arg_slide(int index, std::uint64_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	static_assert(sizeof(std::int64_t) == sizeof(sqlite_int64),
	              "'sqlite_int64' and 'std::int64_t' must have the same sizes!");
	const auto sqlite_result = sqlite3_bind_int64(stmt_.get(), index, unsigned_to_signed_int64(value));
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_arg(int index, std::int64_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	static_assert(sizeof(std::int64_t) == sizeof(sqlite_int64),
	              "'sqlite_int64' and 'std::int64_t' must have the same sizes!");
	const auto sqlite_result = sqlite3_bind_int64(stmt_.get(), index, value);
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_arg_extend(int index, std::uint32_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	const auto sqlite_result = sqlite3_bind_int64(stmt_.get(), index, static_cast<sqlite_int64>(value));
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_arg_cast(int index, std::uint32_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	const auto sqlite_result = sqlite3_bind_int(stmt_.get(), index, static_cast<int>(value));
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_arg_throw(int index, uint32_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	if (value > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
		throw OutOfBoundsError("Value (" + std::to_string(value) + ") in binding is out of bounds");
	}
	const auto sqlite_result = sqlite3_bind_int64(stmt_.get(), index, static_cast<int>(value));
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_arg(int index, int32_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	static_assert(sizeof(std::int32_t) == sizeof(int),
	              "'std::int32_t' and 'int' must have the same sizes!");
	const auto sqlite_result = sqlite3_bind_int(stmt_.get(), index, value);
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_arg_extend(int index, uint16_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	const auto sqlite_result = sqlite3_bind_int(stmt_.get(), index, static_cast<int>(value));
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_arg_extend(int index, uint8_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	const auto sqlite_result = sqlite3_bind_int(stmt_.get(), index, static_cast<int>(value));
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_arg_extend(int index, int16_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	const auto sqlite_result = sqlite3_bind_int(stmt_.get(), index, static_cast<int>(value));
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_arg_extend(int index, int8_t value, const char* name)
{
	DEBUG_LOG( "?" << index << "=" << std::hex << value << std::dec);
	const auto sqlite_result = sqlite3_bind_int(stmt_.get(), index, static_cast<int>(value));
	if (sqlite_result) {
		throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

void Statement::bind_text(int index, const std::string& value, const char* name)
{
	bind_text(index, value.c_str(), value.size(), name);
}

void Statement::bind_text(int index, const char* value, std::size_t size, const char* name)
{
	bind_text_impl(index, value, size, name, true);
}

void Statement::bind_text_without_copy(int index, const std::string& value, const char* name)
{
	bind_text_without_copy(index, value.c_str(), value.size(), name);
}

void Statement::bind_text_without_copy(int index, const char* value, std::size_t size, const char* name)
{
	bind_text_impl(index, value, size, name, false);
}

void Statement::bind_blob(int index, const uint8_t* value, std::size_t size, const char* name)
{
	bind_blob_impl(index, value, size, name, true);
}

void Statement::bind_blob_without_copy(int index, const uint8_t* value, std::size_t size, const char* name)
{
	bind_blob_impl(index, value, size, name, false);
}

void Statement::bind_blob_impl(int index, const uint8_t* value, std::size_t size, const char* name, bool is_transient)
{
	DEBUG_LOG( "?" << index << "=" << value);
	if (size > std::numeric_limits<int>::max()) {
		const auto sqlite_result = sqlite3_bind_blob64(
			stmt_.get(),
			index, value, size,
			is_transient ? SQLITE_TRANSIENT : SQLITE_STATIC
		);

		if (sqlite_result) {
			throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
		}
	} else {
		const auto sqlite_result = sqlite3_bind_blob(
			stmt_.get(),
			index, value, static_cast<int>(size),
			is_transient ? SQLITE_TRANSIENT : SQLITE_STATIC
		);

		if (sqlite_result) {
			throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
		}
	}
}

void Statement::bind_text_impl(int index, const char* value, std::size_t size, const char* name, bool is_transient)
{
	DEBUG_LOG( "?" << index << "=" << value);
	if (size > std::numeric_limits<int>::max()) {
		const auto sqlite_result = sqlite3_bind_text64(
			stmt_.get(),
			index, value, size,
			is_transient ? SQLITE_TRANSIENT : SQLITE_STATIC,
			SQLITE_UTF8
		);

		if (sqlite_result) {
			throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
		}
	} else {
		const auto sqlite_result = sqlite3_bind_text(
			stmt_.get(),
			index, value, static_cast<int>(size),
			is_transient ? SQLITE_TRANSIENT : SQLITE_STATIC);

		if (sqlite_result) {
			throw DatabaseError("Can't bind "s + name + ": " + sqlite3_errstr(sqlite_result));
		}
	}
}

void Statement::bind_null(int index, const char* name)
{
	DEBUG_LOG( "?" << index << "= NULL" << std::dec);
	const auto sqlite_result = sqlite3_bind_null(stmt_.get(), index);
	if (sqlite_result) {
		throw DatabaseError(std::string("Can't bind null to ") + name + ": " + sqlite3_errstr(sqlite_result));
	}
}

Statement::Type Statement::column_type(int column)
{
	return to_type(sqlite3_column_type(stmt_.get(), column));
}

std::int64_t Statement::column_i64(int column)
{
	return sqlite3_column_int64(stmt_.get(), column);
}

std::uint64_t Statement::column_u64(int column)
{
	return static_cast<std::uint64_t>(sqlite3_column_int64(stmt_.get(), column));
}

std::uint64_t Statement::column_u64_slide(int column){
	return signed_to_unsigned_int64(sqlite3_column_int64(stmt_.get(), column));
}

std::int32_t Statement::column_i32(int column)
{
	return sqlite3_column_int(stmt_.get(), column);
}

std::uint32_t Statement::column_u32(int column)
{
	return static_cast<std::uint32_t>(sqlite3_column_int(stmt_.get(), column));
}

std::string Statement::column_text(int column)
{
	return std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt_.get(), column)));
}

std::tuple<const void*, std::size_t> Statement::column_blob(int column)
{
	return { sqlite3_column_blob(stmt_.get(), column), sqlite3_column_bytes(stmt_.get(), column) };
}

Statement::StepResult Statement::step()
{
	const auto sqlite_result = sqlite3_step(stmt_.get());
	switch (sqlite_result) {
	case SQLITE_ROW:
		return Statement::StepResult::Row;
	case SQLITE_DONE:
		return Statement::StepResult::Done;
	case SQLITE_BUSY:
		throw DatabaseBusy("Database busy: "s + sqlite3_errstr(sqlite_result));
	case SQLITE_MISUSE:
		throw std::logic_error("Statement misuse: "s + sqlite3_errstr(sqlite_result));
	default:
		throw DatabaseError("Database error: "s + sqlite3_errstr(sqlite_result));
	}
}

void Statement::reset()
{
	sqlite3_reset(stmt_.get());
}

void Statement::clear_bindings()
{
	sqlite3_clear_bindings(stmt_.get());
}

}} // namespace reven::sqlite
