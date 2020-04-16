#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <stdexcept>

#include "sqlite.h"
#include "query.h"

namespace reven {
namespace sqlite {

constexpr std::uint32_t metadata_version = 1;

///
/// Root metadata exception. Catch this exception to catch all exceptions related to metadata.
///
class MetadataError : public std::runtime_error {
public:
	MetadataError(const char* msg) : std::runtime_error(msg) {}
};

///
/// Exception that occurs when it is not possible to write the metadata.
///
class WriteMetadataError : public MetadataError {
public:
	WriteMetadataError(const char* msg) : MetadataError(msg) {}
};

///
/// Exception that occurs when it is not possible to read the metadata from a database.
///
class ReadMetadataError : public MetadataError {
public:
	ReadMetadataError(const char* msg) : MetadataError(msg) {}
};

///
/// Raw Metadata class that contains the metadata in the format stored and retrieved by the database.
/// It is not to be used directly by clients, instead use converters that give a semantics to the metadata
///
class Metadata {
public:
	/// Magic representing the resource type
	std::uint32_t type() const { return type_; }
	/// A version for the resource file format (should be of format "x.y.z-suffix" with an optional suffix)
	const std::string& format_version() const { return format_version_; }
	/// Name of the tool that generated the resource
	const std::string& tool_name() const { return tool_name_; }
	/// A version for the tool (should be of format "x.y.z-suffix" with an optional suffix)
	const std::string& tool_version() const { return tool_version_; }
	/// The version of the tool and possibly the version of the writer library used
	const std::string& tool_info() const { return tool_info_; }
	/// The date of the generation
	std::uint64_t generation_date() const { return generation_date_; }

private:
	// General clients are not expected to be able to build Metadata
	Metadata() = default;

	std::uint32_t type_;
	std::string format_version_;
	std::string tool_name_;
	std::string tool_version_;
	std::string tool_info_;
	std::uint64_t generation_date_;

	// Special class that is allowed to build Metadata
	friend class MetadataWriter;
	// Special permission for ResourceDatabase to build Metadata
	friend class ResourceDatabase;
};

///
/// Special purpose class that is meant to be subclassed to create classes able to build Metadata.
/// Subclasses can call the static method `write` to build a Metadata.
///
/// Example:
///
/// ```cpp
/// class TestMDWriter : MetadataWriter {
/// public:
/// 	static constexpr std::uint32_t type = 42;
/// 	static constexpr char format_version[] = "1.0.0-dummy";
/// 	static constexpr char tool_name[] = "TestMetaDataWriter";
/// 	static constexpr char tool_version[] = "1.0.0-dummy";
/// 	static constexpr char tool_info[] = "Tests version 1.0.0";
/// 	static constexpr std::uint64_t generation_date = 42424242;
///
/// 	static Metadata dummy_md() {
/// 		return write(type, format_version, tool_name, tool_version, tool_info, generation_date);
/// 	}
/// };
///
/// // clients can then call `TestMDWriter::dummy_md()` to build metadata.
/// ```
class MetadataWriter {
protected:
	static Metadata write(std::uint32_t type, std::string format_version,
	                      std::string tool_name, std::string tool_version, std::string tool_info,
	                      std::uint64_t generation_date) {
		Metadata md;
		md.type_ = type;
		md.format_version_ = std::move(format_version);
		md.tool_name_ = std::move(tool_name);
		md.tool_version_ = std::move(tool_version);
		md.tool_info_ = std::move(tool_info);
		md.generation_date_ = generation_date;
		return md;
	}
};

///
/// Encapsulates a database that contains metadata. Provides methods to retrieve the metadata,
///   and to create a new database with metadata.
/// Since ResourceDatabase inherits from Database, it can be used like a normal Database
class ResourceDatabase : public Database {
public:
	///
	/// \brief open Open a ResourceDatabase located at the specified filename
	/// \param filename Full path to the database.
	///   The file at this location must exist and correspond to a sqlite database that contains metadata.
	/// \param read_only If true, open this database only for reading. Otherwise, open for reading and writing
	/// \throws DatabaseError if the database does not exist
	/// \throws ReadMetadataError if the metadata of this database cannot be read
	static ResourceDatabase open(const char* filename, bool read_only = true);

	/// \brief create Create a new ResourceDatabase at the specified filename with the specified metadata
	/// \param filename Full path to the database to be created.
	///   The containing directory must exist, and should not correspond to an existing sqlite database.
	/// \param metadata Metadata to write
	/// \throws DatabaseError if the containing directory does not exist
	/// \throws WriteMetadataError if the database already exists
	static ResourceDatabase create(const char* filename, Metadata metadata);

	///
	/// \brief from_memory Create a new private ResourceDatabase in memory with the specified metadata
	static ResourceDatabase from_memory(Metadata metadata);

	///
	/// \brief from_db Attempt to downcast a database to a resourcedatabase.
	/// \param db Database to downcast. The database must already contain metadata to read.
	/// \throw ReadMetadataError if the passed database does not contain any metadata to read.
	/// \throw DatabaseError in case of transient I/O error during the operation.
	static ResourceDatabase from_db(Database db);

	///
	/// \brief convert_db Convert a database by adding metadata to it.
	/// \param db Database to convert. The database shall not contain metadata before the operation.
	/// \param metadata Metadata that the database will contain after the operation.
	/// \throw WriteMetadataError if the passed database already contains metadata.
	/// \throw DatabaseError in case of transient I/O error during the operation.
	static ResourceDatabase convert_db(Database db, Metadata metadata);

	///
	/// \brief metadata get the metadata of this database
	const Metadata& metadata() const { return md_; }

	///
	/// \brief set_metadata Updates the metadata of this database
	/// \param md New metadata that will replace the previous metadata
	/// \throw DatabaseError in case of transient I/O error during the operation.
	void set_metadata(Metadata metadata);
private:
	// Cached metadata to avoid rereading from the base
	Metadata md_;
	std::uint32_t md_version_;

	ResourceDatabase(const char* filename, OpenMode mode);

	ResourceDatabase(Database db);

	void create_metadata(const Metadata& metadata);

	void update_metadata(const Metadata& metadata);

	struct VersionedMetadata {
		std::uint32_t version;
		Metadata metadata;
	};

	VersionedMetadata read_metadata();
};

inline ResourceDatabase ResourceDatabase::open(const char* filename, bool read_only)
{
	auto rdb = ResourceDatabase(filename, read_only ? OpenMode::ReadOnly : OpenMode::ReadWrite);
	auto result = rdb.read_metadata();
	rdb.md_ = std::move(result.metadata);
	rdb.md_version_ = result.version;
	return rdb;
}

inline ResourceDatabase ResourceDatabase::create(const char* filename, Metadata metadata)
{
	auto rdb = ResourceDatabase(filename, OpenMode::Create);
	rdb.create_metadata(metadata); // copy
	rdb.md_ = std::move(metadata);
	rdb.md_version_ = metadata_version;
	return rdb;
}

inline ResourceDatabase ResourceDatabase::from_memory(Metadata metadata)
{
	auto db = Database::from_memory();
	auto rdb = ResourceDatabase(std::move(db));
	rdb.create_metadata(metadata); // copy
	rdb.md_ = std::move(metadata);
	rdb.md_version_ = metadata_version;
	return rdb;
}

inline ResourceDatabase ResourceDatabase::from_db(Database db)
{
	auto rdb = ResourceDatabase(std::move(db));
	auto result = rdb.read_metadata();
	rdb.md_ = std::move(result.metadata);
	rdb.md_version_ = result.version;
	return rdb;
}

inline ResourceDatabase ResourceDatabase::convert_db(Database db, Metadata metadata)
{
	auto rdb = ResourceDatabase(std::move(db));
	rdb.create_metadata(std::move(metadata)); // copy
	rdb.md_ = std::move(metadata);
	rdb.md_version_ = metadata_version;
	return rdb;
}

inline void ResourceDatabase::set_metadata(Metadata metadata)
{
	if (md_version_ != metadata_version) {
		throw WriteMetadataError("Can't set the metadata with different metadata version than the current");
	}

	update_metadata(metadata); // copy
	md_ = std::move(metadata);
}

inline ResourceDatabase::ResourceDatabase(const char* filename, Database::OpenMode mode) : Database(filename, mode)
{}

inline ResourceDatabase::ResourceDatabase(Database db) : Database(std::move(db))
{}

}} // namespace reven::sqlite
