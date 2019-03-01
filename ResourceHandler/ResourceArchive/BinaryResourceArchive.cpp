#include "BinaryResourceArchive.h"
#include <filesystem>
#include <Profiler.h>
#include <FStreamHelpers.h>

namespace fs = std::experimental::filesystem;
ResourceArchive::BinaryResourceArchive::Archive::Archive( const std::string & archivePath, ArchiveMode mode )
{
	auto m = std::ios::binary | std::ios::in | std::ios::ate;
	if ( mode == ArchiveMode::development )
		m |= std::ios::out;

	stream.open( archivePath, m );
	if ( !stream.is_open() )
	{
		if ( fs::exists( archivePath ) )
			throw ArchivePathNotAccessible( archivePath );
		else if ( mode != ArchiveMode::development )
			throw ArchivePathNotFound( archivePath );

		std::ofstream newArchive( archivePath, std::ios::binary );
		if ( !newArchive.is_open() )
			throw UNKOWN_ERROR;

		header.version = lastestVersion;
		header.tailSize = 0;
		header.tailStart = sizeof( header );
		Utilities::Binary::write( newArchive, header );
		newArchive.close();

		stream.open( archivePath, m );
		if ( !stream.is_open() )
			throw UNKOWN_ERROR;
	}

	size_t totalFileSize = size_t( stream.tellg() );
	if ( totalFileSize < sizeof( header ) )
		throw ArchiveCorrupt( archivePath, 0 );

	readHeader();

	if ( header.tailStart > totalFileSize )
		throw ArchiveCorrupt( archivePath, 1 );
	else if ( header.tailStart < sizeof( header ) )
		throw ArchiveCorrupt( archivePath, 2 );
	else if ( header.tailStart + header.tailSize != totalFileSize )
		throw ArchiveCorrupt( archivePath, 3 );

	readTail();
}

ResourceArchive::BinaryResourceArchive::Archive::~Archive()
{
	stream.close();
}

void ResourceArchive::BinaryResourceArchive::Archive::readHeader()
{
	stream.seekg( 0 );
	Utilities::Binary::read( stream, header );
}

void ResourceArchive::BinaryResourceArchive::Archive::readTail()
{
	stream.seekg( header.tailStart );
	entries.readFromFile( stream );
}

void ResourceArchive::BinaryResourceArchive::Archive::writeHeader()
{
	stream.seekg( 0 );
	Utilities::Binary::write( stream, header );
}


ResourceArchive::BinaryResourceArchive::BinaryResourceArchive( const std::string& archivePath, ArchiveMode mode ) : _archive( archivePath, mode )
{

}


ResourceArchive::BinaryResourceArchive::~BinaryResourceArchive()
{

}

bool ResourceArchive::BinaryResourceArchive::exists( Utilities::GUID ID ) const noexcept
{
	return _archive.entries.find( ID ).has_value();
}

size_t ResourceArchive::BinaryResourceArchive::size( Utilities::GUID ID ) const
{
	return _archive.entries.find( ID ).or_else( [&]
	{
		throw ArchiveResourceNotFound( ID );
	} ).map( [&]( size_t index )
	{
		return _archive.entries.peek<Archive::Entries::DataSize>( index );
	} ).value();
}

std::string_view ResourceArchive::BinaryResourceArchive::name( Utilities::GUID ID ) const
{
	return  _archive.entries.find( ID ).or_else( [&]
	{
		throw ArchiveResourceNotFound( ID );
	} ).map( [&]( size_t index )
	{
		return _archive.entries.peek<Archive::Entries::Name>( index );
	} ).value();
}

const ResourceArchive::ArchiveInfo& ResourceArchive::BinaryResourceArchive::read( Utilities::GUID ID )
{
	return _archive.entries.find( ID ).or_else( [&]
	{
		throw ArchiveResourceNotFound( ID );
	} ).map( [&]( size_t index )
	{
		return _archive.loadEntry( ID, index );
	} ).value();
}

void ResourceArchive::BinaryResourceArchive::write( Utilities::GUID ID, Memory_Block & memory )
{
	if ( mode != ArchiveMode::development )
		throw ArchiveNotInDeveloperMode( ID );

}


const ResourceArchive::ArchiveInfo& ResourceArchive::BinaryResourceArchive::Archive::loadEntry( Utilities::GUID ID, size_t index )
{
	if ( auto find = loadedEntries.find( ID ); find != loadedEntries.end() )
		return find->second;

	auto& info = loadedEntries[ID];
	info.ID = ID;
	info.name = entries.get<Entries::Name>( index );
	stream.seekg( entries.get<Entries::DataStart>( index ) );

	info.memory.size = entries.get<Entries::DataSize>( index );
	info.memory.data = operator new(info.memory.size);

	Utilities::Binary::read( stream, info.memory.data, info.memory.size );

	return info;
}

