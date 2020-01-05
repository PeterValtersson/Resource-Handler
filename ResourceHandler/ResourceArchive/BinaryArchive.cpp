#include "BinaryArchive.h"
#include <filesystem>
#include <Utilities/Profiler/Profiler.h>
#include <Utilities/FStreamHelpers.h>

namespace fs = std::filesystem;
Resources::BinaryArchive::BinaryArchive( std::string_view archivePath, AccessMode mode ) : archivePath( archivePath )
{
	PROFILE;
	auto m = std::ios::binary | std::ios::in | std::ios::ate;
	if ( mode == AccessMode::read_write )
		m |= std::ios::out;

	stream.open( archivePath, m );
	if ( !stream.is_open() )
	{
		if ( fs::exists( archivePath ) )
			throw PathNotAccessible( archivePath );
		else if ( mode != AccessMode::read_write )
			throw PathNotFound( archivePath );

		std::ofstream newArchive( archivePath, std::ios::binary );
		if ( !newArchive.is_open() )
			throw UNKOWN_ERROR;

		header.version = lastestVersion;
		header.tailStart = sizeof( header );
		header.unusedSpace = 0;
		Utilities::Binary_Stream::write( newArchive, header );
		entries.writeToFile( newArchive );

		newArchive.close();

		stream.open( archivePath, m );
		if ( !stream.is_open() )
			throw UNKOWN_ERROR;
	}

	size_t totalFileSize = size_t( stream.tellg() );
	if ( totalFileSize < sizeof( header ) )
		throw ArchiveCorrupt( 0 );

	readHeader();

	if ( header.tailStart > totalFileSize )
		throw ArchiveCorrupt( 1 );
	else if ( header.tailStart < sizeof( header ) )
		throw ArchiveCorrupt( 2 );
	else if ( header.unusedSpace > totalFileSize )
		throw ArchiveCorrupt( 3 );

	readTail();
}

Resources::BinaryArchive::~BinaryArchive()
{
	stream.close();
}

const size_t Resources::BinaryArchive::num_resources() const noexcept
{
	return entries.size();
}

Utilities::GUID Resources::BinaryArchive::create_from_name( std::string_view name )
{
	PROFILE;
	entries.add( Utilities::GUID(name), name, 0, 0, 0 );
	return Utilities::GUID( name );
}

void Resources::BinaryArchive::create_from_ID( const Utilities::GUID ID )
{}

void Resources::BinaryArchive::create( const Utilities::GUID ID, std::string_view name )
{}

void Resources::BinaryArchive::save_resource_info()
{
	PROFILE;
	writeHeader();
	writeTail();
}

void Resources::BinaryArchive::save( const To_Save& to_save, Utilities::Memory::ChunkyAllocator& allocator )
{
	PROFILE;
	try
	{
		_save_resource_info_data( to_save, allocator );
	}
	catch ( ... )
	{
		save_resource_info();
	}
	save_resource_info();
}

void Resources::BinaryArchive::save_multiple( const To_Save_Vector& to_save_vector, Utilities::Memory::ChunkyAllocator& allocator )
{
	PROFILE;
	for ( const auto& to_save : to_save_vector )
	{
		try
		{
			_save_resource_info_data( to_save, allocator );
		}
		catch ( ... )
		{
			save_resource_info();
		}
	}
	save_resource_info();
}

void Resources::BinaryArchive::_save_resource_info_data( const To_Save& to_save, Utilities::Memory::ChunkyAllocator& allocator )
{
	PROFILE;
	if ( const auto entry = entries.find( to_save.first ); !entry.has_value() )
		throw ResourceNotFound( to_save.first );
	else
	{
		allocator.peek_data( to_save.second, [&]( const Utilities::Memory::ConstMemoryBlock mem )
		{
			if ( mem.used_size <= entries.peek<Entries::DataSize>( *entry ) )
			{
				stream.seekp( entries.peek<Entries::DataStart>( *entry ) );
				mem.write_to_stream( stream );
				entries.set<Entries::DataSize>( *entry, mem.used_size );
			}
			else
			{
				stream.seekp( header.tailStart );
				mem.write_to_stream( stream );
				entries.set<Entries::DataStart>( *entry, header.tailStart );
				entries.set<Entries::DataSize>( *entry, mem.used_size );
				header.tailStart = stream.tellp();
			}

		} );
	}
}

void Resources::BinaryArchive::readHeader()
{
	PROFILE;
	stream.seekg( 0 );
	Utilities::Binary_Stream::read( stream, header );
}

void Resources::BinaryArchive::readTail()
{
	PROFILE;
	stream.seekg( header.tailStart );
	auto result = entries.readFromFile( stream );
	if ( result < 0 )
		throw ArchiveCorrupt( result );
}


void Resources::BinaryArchive::writeHeader()
{
	PROFILE;
	stream.seekp( 0 );
	Utilities::Binary_Stream::write( stream, header );
}

void Resources::BinaryArchive::writeTail()
{
	PROFILE;
	stream.seekp( header.tailStart );
	entries.writeToFile( stream );
}

const bool Resources::BinaryArchive::exists( const Utilities::GUID ID ) const noexcept
{
	PROFILE;
	return entries.find( ID ).has_value();
}

const size_t Resources::BinaryArchive::get_size( const Utilities::GUID ID ) const
{
	PROFILE;
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ResourceNotFound( ID );
	else
		return entries.peek<Entries::DataSize>( *entry );
}

const std::string Resources::BinaryArchive::get_name( const Utilities::GUID ID ) const
{
	PROFILE;
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ResourceNotFound( ID );
	else
		return entries.peek<Entries::Name>( *entry );
}

const Utilities::GUID Resources::BinaryArchive::get_type( const Utilities::GUID ID ) const
{
	PROFILE;
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ResourceNotFound( ID );
	else
		return entries.peek<Entries::Type>( *entry );
}

void Resources::BinaryArchive::set_name( const Utilities::GUID ID, std::string_view name )
{
	PROFILE;
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ResourceNotFound( ID );
	else
		memcpy( &entries.get<Entries::Name>( *entry ), name.data(), name.size() + 1 );
}

void Resources::BinaryArchive::set_type( const Utilities::GUID ID, const Utilities::GUID type )
{
	PROFILE;
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ResourceNotFound( ID );
	else
		entries.set<Entries::Type>( *entry, type );
}

const Utilities::Memory::Handle Resources::BinaryArchive::read( const Utilities::GUID ID, Utilities::Memory::ChunkyAllocator& allocator )
{
	PROFILE;
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ResourceNotFound( ID );
	else
	{
		auto size = entries.get<Entries::DataSize>( *entry );
		auto handle = allocator.allocate( size );
		stream.seekg( entries.get<Entries::DataStart>( *entry ) );
		allocator.use_data( handle, [&]( Utilities::Memory::MemoryBlock data )
		{
			data.read_from_stream( stream );
		} );
		return handle;
	}
}