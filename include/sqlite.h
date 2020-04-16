#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <stdexcept>

struct sqlite3;
struct sqlite3_stmt;


namespace reven {
namespace sqlite {

///
/// Module-level Exception.
///
class DatabaseError : public std::runtime_error {
public:
	DatabaseError(const char* c) : std::runtime_error(c) {}
	DatabaseError(const std::string& c) : std::runtime_error(c) {}
};

class DatabaseNotFound : public DatabaseError {
public:
	DatabaseNotFound(const char* c) : DatabaseError(c) {}
	DatabaseNotFound(const std::string& c) : DatabaseError(c) {}
};

///
/// Thrown on some operations if the database is busy due to another process.
///
class DatabaseBusy : public DatabaseError {
public:
	DatabaseBusy(const char* c) : DatabaseError(c) {}
	DatabaseBusy(const std::string& c) : DatabaseError(c) {}
};

class OutOfBoundsError : public DatabaseError {
public:
	OutOfBoundsError(const char* c) : DatabaseError(c) {}
	OutOfBoundsError(const std::string& c) : DatabaseError(c) {}
};

///
/// Thin wrapper around a sqlite3 object (that represents sqlite3 database connection).
///
/// Handles creation and destruction of the database, and exposes methods that binds the sqlite3 functions
/// operating on database as methods.
///
class Database {
public:
	enum class OpenMode {
		Create, ///<- Creates DB, allows R+W
		ReadWrite, ///<- Allows R+W
		ReadOnly ///<- Only allows R
	};

	/// \brief Opens a new database connection from a filename and a mode
	/// \param filename Path where to locate the database.
	/// \param mode
	///
	/// @note * If the mode is Create, the database will be created if it doesn't exist, and truncated if it exists
	///   * If the mode is not Create, the path needs to refer to an existing database.
	///
	/// @throws DatabaseError if the database cannot be opened/created
	Database(const char* filename, OpenMode mode);

	///
	/// \brief Takes ownership of an existing database connection
	/// \param raw_db Existing database connection
	///
	/// @note The resulting object takes ownership of the passed database connection, meaning that it will now manage
	///   its resource and call sqlite3_close on it.
	///   In particular, this means that two pointers to the same connection should not be owned by two Database objects
	///   at once.
	Database(sqlite3* raw_db);

	///
	/// \brief from_memory Creates a new database in memory and returns a connection to it
	///
	/// @note Two calls to the function will result in connections to two different databases.
	static Database from_memory() { return Database(":memory:", OpenMode::Create); }

	///
	/// \brief get Get the underlying raw database connection
	///
	/// @note The resulting pointer refers to a connection still owned by the current instance and should not be
	///   manually freed, nor sent to another Database instance
	sqlite3* get() { return db_.get(); }
	const sqlite3* get() const { return db_.get(); }

	///
	/// \brief release Relinquish ownership of the underlying raw database connection and returns it
	///
	/// @note The resulting pointer refers to a connection not owned by the current instance, and should be manually
	///   freed or sent to another Database instance
	sqlite3* release() && { return db_.release(); }

	///
	/// \brief exec Execute a SQL command on the database
	/// \param command Command to execute
	/// \param error_message Error message to transmit to the exception thrown if the command is not successful.
	///
	/// @throws DatabaseError if the command doesn't execute correctly
	///   (database is busy, I/O error, invalid command, ...)
	void exec(const char* command, const char* error_message);

	std::int64_t last_insert_rowid() const;
private:

	using UniqueDBPtr = std::unique_ptr<sqlite3, std::function<void(sqlite3*)>>;

	UniqueDBPtr db_;
};

///
/// Thin wrapper around a sqlite3_stmt object (that represent a sqlite3 prepared statement).
///
/// Handles creation and destruction of the statement, and exposes methods that binds the sqlite3 functions
/// operating on statements as methods.
///
class Statement {
public:
	///
	/// Result of a call to step()
	///
	enum class StepResult {
		Done, ///<- The statement doesn't have a row. Call reset() on it to reuse it.
		Row ///<- The statement has a row. Fetch it with column_ methods or get to the next row with step()
	};

	/// SQL types
	enum class Type {
		Integer,
		Float,
		Text,
		Blob,
		Null
	};

	///
	/// \brief Statement Creates a prepared statement attached to a database
	/// \param db Connection to the database to which attach the statement
	/// \param stmt_str SQL text of the statement
	///
	/// @note lifetime(Statement) < lifetime(Database)
	/// @throws DatabaseError if the statement cannot be prepared
	Statement(Database& db, const char* stmt_str);

	/// Binding methods
	///
	/// \brief bind_arg Attempts to bind a value to the prepared statement
	/// \param column Index of the "?" character to replace in the prepared statement
	/// \param value Value by which the "?" character should be replaced
	/// \param name Name of the bound value, to transmit to the exception thrown if the binding fails
	///
	/// @note Indexes start at 1, not 0 (unlike indexes in column_type)
	/// @warning Internally, the database works with signed integers. Because of that, several variants are provided to
	///   handle the case where the unsigned value is not representable as the signed value.
	///   * The "_cast" variant simply casts the passed value, which is fine if the value is in range, but otherwise
	///     results in implementation-defined behavior. Typically, this means that an unsigned integer outside of the
	///     range of signed integer of the same size will be casted to negative integer.
	///   * The "_throw" variant checks whether the value is in bounds, and throws if it is not the case.
	///     You can use this one if you want to use the bound values in inequalities.
	///   * The "_extend" variant will cast to a type where the value can always be represented. It is the case for
	///     (signed and unsigned) integers of size 16 and below, as the lowest type in sqlite is the 32-bits int.
	///   * The "_slide" variant is a special case for 64-bit unsigned value.
	///     It will slide any value of the unsigned range to the corresponding value in the signed range.
	///     It means the number "order" is preserved, unlike the "_cast" variant. This will allow comparison of 64-bit unsigned integer in query.
	///     **Important note**: when a value is inserted with this "_slide" variant, the value has to be retrieved with `column_i64_to_uint64` in order to slide back and get the real uint64 value.
	///   * The unsuffixed variants don't perform any cast.
	/// @throw DatabaseError if the binding fails
	/// @throw OutOfBoundsError if the passed value is out of bounds (in a "_throw" variant)
	/// @{
	void bind_arg_cast(int index, std::uint64_t value, const char* name);
	void bind_arg_throw(int index, std::uint64_t value, const char* name);
	void bind_arg_slide(int index, std::uint64_t value, const char* name);
	void bind_arg(int index, std::int64_t value, const char* name);

	void bind_arg_extend(int index, std::uint32_t value, const char* name); // bound to int64
	void bind_arg_cast(int index, std::uint32_t value, const char* name);
	void bind_arg_throw(int index, std::uint32_t value, const char* name);
	void bind_arg(int index, std::int32_t value, const char* name);

	void bind_arg_extend(int index, std::uint16_t value, const char* name);
	void bind_arg_extend(int index, std::uint8_t value, const char* name);
	void bind_arg_extend(int index, std::int16_t value, const char* name);
	void bind_arg_extend(int index, std::int8_t value, const char* name);

	/// In bind_test(_X) function:
	/// @param size size of value
	/// @note: consider using std::string_view for overloads std::string& when C++17
	void bind_text(int index, const std::string& value, const char* name);
	void bind_text(int index, const char* value, std::size_t size, const char* name);

	/// Calling this function creates a binding to the passed string reference.
	/// @warning With this overload, no copy of the passed reference is made, so the passed reference must outlive the
	///   binding.
	/// To clear the binding, use clear_bindings() or rebind this index.
	/// If the string reference cannot outlive the binding, use the copy overload.
	void bind_text_without_copy(int index, const std::string& value, const char* name);
	void bind_text_without_copy(int index, const char* value, std::size_t size, const char* name);

	void bind_blob(int index, const std::uint8_t* value, std::size_t size, const char* name);
	/// @warning With this overload, no copy of the passed reference is made, so the passed reference must outlive the
	///   binding.
	void bind_blob_without_copy(int index, const std::uint8_t* value, std::size_t size, const char* name);

	void bind_null(int index, const char* name);
	/// @}

	///
	/// \brief column_type Gets the SQL type of a column in the current row of this statement
	/// \param column Index of the column to fetch
	///
	/// @note Indexes start at 0, not 1 (unlike indexes in bind_arg)
	/// @warning If no column has this index, or if the statement has no current row, then the result is undefined
	Type column_type(int column);

	///
	/// \brief column_i64 Gets the value of a column in the current row of this statement as a 64-bit integer
	/// \param column Index of the column to fetch
	///
	/// @note Indexes start at 0, not 1 (unlike indexes in bind_arg)
	/// @warning If no column has this index, or if the statement has no current row, then the result is undefined
	std::int64_t column_i64(int column);

	///
	/// \brief column_u64 Gets the value of a column in the current row of this statement as a 64-bit unsigned integer
	/// \param column Index of the column to fetch
	///
	/// @note Indexes start at 0, not 1 (unlike indexes in bind_arg)
	/// @warning If no column has this index, or if the statement has no current row, then the result is undefined
	std::uint64_t column_u64(int column);

	///
	/// \brief column_i64_to_u64 Gets the value of a column in the current row of this statement as a 64-bit signed integer and transform it into an 64-bit unsigned integer
	/// \param column Index of the column to fetch
	///
	/// @note Indexes start at 0, not 1 (unlike indexes in bind_arg)
	/// @warning If no column has this index, or if the statement has no current row, then the result is undefined
	/// @note **Important**: this method should only retrieve value from column using `bind_arg_slide` method to insert values
	std::uint64_t column_u64_slide(int column);

	///
	/// \brief column_i32 Gets the value of a column in the current row of this statement as a 32-bit integer
	/// \param column Index of the column to fetch
	///
	/// @note Indexes start at 0, not 1 (unlike indexes in bind_arg)
	/// @warning If no column has this index, or if the statement has no current row, then the result is undefined
	std::int32_t column_i32(int column);

	///
	/// \brief column_u32 Gets the value of a column in the current row of this statement as a 32-bit unsigned integer
	/// \param column Index of the column to fetch
	///
	/// @note Indexes start at 0, not 1 (unlike indexes in bind_arg)
	/// @warning If no column has this index, or if the statement has no current row, then the result is undefined
	std::uint32_t column_u32(int column);

	///
	/// \brief column_text Gets the value of a column in the current row of this statement as a std::string
	/// \param column Index of the column to fetch
	///
	/// @note Indexes start at 0, not 1 (unlike indexes in bind_arg)
	/// @warning If no column has this index, or if the statement has no current row, then the result is undefined
	std::string column_text(int column);

	/// @warning Returned pointer lifetime is valid until the next call to step, reset or the current instance is
	///  destroyed
	std::tuple<const void*, std::size_t> column_blob(int column);

	///
	/// \brief step Executes the statement, fetching the next row if any.
	/// \return An StepResult indicating whether the statement has a row, whether it is done, etc.
	///
	/// @note Does not throw DatabaseError on runtime errors, but returns StepResult::Busy or StepResult::Error
	/// @throws std::logic_error If step is called on a statement that wasn't fully bound, or a statement that wasn't
	///   reset after returning StepResult::Done.
	/// @throws DatabaseBusy If step is called while the database is busy.
	/// @throws DatabaseError If another kind of error happens (IO error, etc).
	StepResult step();

	///
	/// \brief reset Resets the statement so it can be reused. This does not clear the existing bindings.
	///
	void reset();
	///
	/// \brief clear_bindings Clears the bindings on this statement. This does not reset the statement
	///
	void clear_bindings();

private:
	using UniqueStmtPtr = std::unique_ptr<sqlite3_stmt, std::function<void(sqlite3_stmt*)>>;

	void bind_text_impl(int index, const char* value, std::size_t size, const char* name, bool is_transient = true);
	void bind_blob_impl(int index, const uint8_t* value, std::size_t size, const char* name, bool is_transient);

	UniqueStmtPtr stmt_;

};
}} // namespace reven::sqlite
