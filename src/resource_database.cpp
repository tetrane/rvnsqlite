#include "resource_database.h"

#include <sqlite3.h>

namespace reven {
namespace sqlite {

namespace {
void write_metadata(Statement& stmt, const Metadata& metadata)
{
	// bind arguments
	stmt.bind_arg_cast(2, metadata.type(), "type");
	stmt.bind_text_without_copy(3, metadata.format_version(), "format version");
	stmt.bind_text_without_copy(4, metadata.tool_name(), "tool name");
	stmt.bind_text_without_copy(5, metadata.tool_version(), "tool version");
	stmt.bind_text_without_copy(6, metadata.tool_info(), "tool info");
	stmt.bind_arg_cast(7, metadata.generation_date(), "generation date");

	stmt.step();
}
} // anonymous namespace

void ResourceDatabase::create_metadata(const Metadata& metadata)
{
	// create table
	try {
		exec("create table _metadata ("
		     "metadata_version int,"
		     "type int,"
		     "format_version text,"
		     "tool_name text,"
		     "tool_version text,"
		     "tool_info text,"
		     "generation_date int8"
		     ");",
		     "could not create metadata table!");

	} catch (DatabaseError&) {
		throw WriteMetadataError("Could not create metadata. "
		                         "Either this is not a database or the metadata already exists");
	}

	// create statement
	Statement stmt(*this, "insert into _metadata values(?,?,?,?,?,?,?);");

	stmt.bind_arg_cast(1, metadata_version, "metadata version");
	write_metadata(stmt, metadata);
}

void ResourceDatabase::update_metadata(const Metadata& metadata)
{
	// create statement
	Statement stmt(*this, "update _metadata set "
	                      "metadata_version = ?,"
	                      "type = ?,"
	                      "format_version = ?,"
	                      "tool_name = ?,"
	                      "tool_version = ?,"
	                      "tool_info = ?,"
	                      "generation_date = ?"
	                      ";");

	stmt.bind_arg_cast(1, metadata_version, "metadata version");
	write_metadata(stmt, metadata);
}

ResourceDatabase::VersionedMetadata ResourceDatabase::read_metadata()
{
	try {
		Statement stmt(*this, "select * from _metadata;");


		if (stmt.step() != Statement::StepResult::Row) {
			throw ReadMetadataError("Ill-formed metadata: no metadata entry");
		}

		std::uint32_t column_counter = 0;

		std::uint32_t metadata_version = 0;
		if (sqlite3_table_column_metadata(get(), nullptr, "_metadata", "metadata_version", nullptr, nullptr, nullptr, nullptr, nullptr) == SQLITE_OK) {
			metadata_version = stmt.column_u32(column_counter++);

			if (metadata_version > ::reven::sqlite::metadata_version) {
				throw ReadMetadataError("Metadata version in the future");
			}
		} else {
			metadata_version = 0;
		}

		Metadata md;

		md.type_ = stmt.column_u32(column_counter++);
		md.format_version_ = stmt.column_text(column_counter++);
		md.tool_name_ = stmt.column_text(column_counter++);

		if (metadata_version >= 1) {
			md.tool_version_ = stmt.column_text(column_counter++);
		} else {
			md.tool_version_ = "1.0.0-prerelease";
		}

		md.tool_info_ = stmt.column_text(column_counter++);
		md.generation_date_ = stmt.column_u64(column_counter++);

		if (stmt.step() != Statement::StepResult::Done) {
			throw ReadMetadataError("Ill-formed metadata: multiple metadata entries");
		}

		return {metadata_version, md};
	} catch (DatabaseError&) {
		throw ReadMetadataError("Missing metadata. Is this a resource database?");
	}
}

}}
