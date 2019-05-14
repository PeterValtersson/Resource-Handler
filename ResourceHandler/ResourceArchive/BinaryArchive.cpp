#include "BinaryArchive.h"
#include <filesystem>
#include <Profiler.h>
#include <FStreamHelpers.h>

namespace fs = std::experimental::filesystem;
Resources::BinaryArchive::BinaryArchive( const std::string & archivePath, AccessMode mode ) : archivePath( archivePath )
{
	Profile;
	auto m = std::ios::binary | std::ios::in | std::ios::ate;
	if ( mode == AccessMode::read_write )
		m |= std::ios::out;

	stream.open( archivePath, m );
	if ( !stream.is_open() )
	{
		if ( fs::exists( archivePath ) )
			throw ArchivePathNotAccessible( archivePath );
		else if ( mode != AccessMode::read_write )
			throw ArchivePathNotFound( archivePath );

		std::ofstream newArchive( archivePath, std::ios::binary );
		if ( !newArchive.is_open() )
			throw UNKOWN_ERROR;

		header.version = lastestVersion;
		header.tailStart = sizeof( header );
		header.unusedSpace = 0;
		Utilities::Binary::write( newArchive, header );
		entries.writeToFile( newArchive );

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
	else if ( header.unusedSpace > totalFileSize )
		throw ArchiveCorrupt( archivePath, 3 );

	readTail();
}

Resources::BinaryArchive::~BinaryArchive()
{
	stream.close();
}

void Resources::BinaryArchive::save( const std::vector<std::pair<Utilities::GUID, const Utilities::Allocators::MemoryBlock>>& data_to_save )
{ 
	for ( auto& to_save : data_to_save )
	{
		if ( auto entry = entries.find( to_save.first ); entry.has_value() )
		{
			if ( to_save.second.used_size <= entries.peek<Entries::DataSize>( *entry ) )
			{
				stream.seekp( entries.peek<Entries::DataStart>( *entry ) );
				stream.write( (char*)to_save.second.data, to_save.second.used_size );
				entries.set<Entries::DataSize>( *entry, to_save.second.used_size );
			}
			else
			{
				stream.seekp( header.tailStart );
				stream.write( (char*)to_save.second.data, to_save.second.used_size );
				entries.set<Entries::DataStart>( *entry, header.tailStart );
				entries.set<Entries::DataSize>( *entry, to_save.second.used_size );
				header.tailStart = stream.tellp();
			}
		}
	}
	writeHeader();
	writeTail();
}

void Resources::BinaryArchive::readHeader()
{
	Profile;
	stream.seekg( 0 );
	Utilities::Binary::read( stream, header );
}

void Resources::BinaryArchive::readTail()
{
	Profile;
	stream.seekg( header.tailStart );
	auto result = entries.readFromFile( stream );
	if ( result < 0 )
		throw ArchiveCorrupt( archivePath, result );
}


void Resources::BinaryArchive::writeHeader()
{
	Profile;
	stream.seekp( 0 );
	Utilities::Binary::write( stream, header );
}

void Resources::BinaryArchive::writeTail()
{
	Profile;
	stream.seekp( header.tailStart );
	entries.writeToFile( stream );
}

bool Resources::BinaryArchive::exists( const Utilities::GUID ID ) const noexcept
{
	Profile;
	return entries.find( ID ).has_value();
}

size_t Resources::BinaryArchive::get_size( const Utilities::GUID ID ) const
{
	Profile;
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ArchiveResourceNotFound( ID );
	else
		return entries.peek<Entries::DataSize>( *entry );
}

std::string Resources::BinaryArchive::get_name( const Utilities::GUID ID ) const
{
	Profile;
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ArchiveResourceNotFound( ID );
	else
		return entries.peek<Entries::Name>( *entry );
}

void Resources::BinaryArchive::set_name( const Utilities::GUID ID, const std::string & name )
{
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ArchiveResourceNotFound( ID );
	else
		memcpy(&entries.get<Entries::Name>( *entry ), name.c_str(), name.size() + 1);
}

void Resources::BinaryArchive::set_type( const Utilities::GUID ID, const Utilities::GUID type )
{
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ArchiveResourceNotFound( ID );
	else
		entries.set<Entries::Type>( *entry, type );
}

const Utilities::Allocators::Handle Resources::BinaryArchive::read( const Utilities::GUID ID, Utilities::Allocators::ChunkyAllocator& allocator )
{
	Profile;
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ArchiveResourceNotFound( ID );
	else
	{
		auto size = entries.get<Entries::DataSize>( *entry );
		auto handle = allocator.allocate( size );
		stream.seekg( entries.get<Entries::DataStart>( *entry ) );
		allocator.use_data( handle, [&]( Utilities::Allocators::MemoryBlock data )
							{
								Utilities::Binary::read( stream, data.data, size );
							} );
		return handle;
	}
}
//
//void Resources::BinaryArchive::write( const Utilities::GUID ID, const Utilities::Allocators::MemoryBlock data )
//{
//	Profile;
//
//	if ( data.size == 0 || data.data == nullptr )
//		throw UNKOWN_ERROR;
//
//	
//		LoadedEntryInfo info;
//		info.ID = ID;
//		info.name = "";
//		info.size = size;
//		info.saved = false;
//		info.memoryHandle = allocator.allocate( size );
//		auto memory = allocator.getData( info.memoryHandle );
//		memcpy( memory.data().data, data, info.size );
//
//		guidToEntryIndex[ID] = loadedEntries.size();
//		loadedEntries.push_back( info );
//	
//	//if ( auto find = entries.find( ID ); find.has_value() )
//	//	writeEntry( ID, memory, entries.get<Entries::Name>( *find ) );
//	//else
//	//	writeEntry( ID, memory, "" );
//}
//
//void Resources::BinaryArchive::set_name( Utilities::GUID ID, const std::string & name )
//{
//	Profile;
//
//	// Already loaded
//
//		LoadedEntryInfo info;
//		info.ID = ID;
//		info.name = name;
//		info.size = 0;
//		info.saved = false;
//
//		guidToEntryIndex[ID] = loadedEntries.size();
//		loadedEntries.push_back( info );
//	
//}

//
//void ResourceArchive::BinaryArchive::writeEntry( Utilities::GUID ID, Memory_Block memory, std::string_view name )
//{
//	entries.find( ID ).map( [&]( size_t index )
//	{	// Entry already exists
//		// If the file is not loaded
//		if ( auto find = loadedEntries.find( ID ); find == loadedEntries.end() )
//		{
//			// Initiate the loaded entry data
//			auto& info = loadedEntries[ID];
//			info.ID = ID;
//			info.name = name;
//			info.memory.size = 0;
//			info.memory.data = operator new(memory.size);
//		}
//
//		auto& info = loadedEntries[ID];
//
//		// If new memory size is larger than old, entry is added at the end
//		if ( memory.size > info.memory.size )
//		{
//			// Reallocate for more space
//			operator delete(info.memory.data);
//			info.memory.data = operator new(memory.size);
//
//			// If this is not the last entry in the file
//			if ( entries.peek<Entries::DataStart>( index ) + info.memory.size < header.tailStart )
//			{
//				// Record the unused space and move the data start
//				header.unusedSpace += info.memory.size;
//				entries.get<Entries::DataStart>( index ) = header.tailStart;
//				header.tailStart += memory.size;
//			}
//			else
//			{
//				// Move tail by the difference
//				header.tailStart += memory.size - info.memory.size;
//			}
//
//		}
//		// New memory size is smaller than old, entry stays at the same location
//		else if ( memory.size < info.memory.size )
//		{
//			// If this is not the last entry in the file
//			if ( entries.peek<Entries::DataStart>( index ) + info.memory.size < header.tailStart )
//			{
//				// Record the unused space
//				header.unusedSpace += info.memory.size - memory.size;
//			}
//			else
//			{
//				// Move tail by the difference
//				header.tailStart -= info.memory.size - memory.size;
//			}
//		}
//
//		// Set the new size
//		entries.get<Entries::DataSize>( index ) = info.memory.size;
//		info.memory.size = memory.size;
//
//		// Copy over the new data
//		memcpy( info.memory.data, memory.data, info.memory.size );
//		info.saved = false;
//
//		return index;
//	} )
//
//		.or_else( [&]
//	{ // Entry does not exist
//
//	  // First add the entry
//		auto index = entries.add( ID );
//		memcpy( entries.get<Entries::Name>( index ), name.data(), name.size() );
//		entries.get<Entries::Name>( index )[name.size()] = '\0';
//		entries.get<Entries::DataStart>( index ) = header.tailStart;
//		entries.get<Entries::DataSize>( index ) = memory.size;
//
//		header.tailStart += memory.size;
//
//		// Initate the loaded entry data
//		auto& info = loadedEntries[ID];
//		info.ID = ID;
//		info.name = name;
//		info.saved = false;
//		info.memory.size = memory.size;
//		info.memory.data = operator new(memory.size);
//		memcpy( info.memory.data, memory.data, info.memory.size );
//	} );
//}
//
//const ResourceArchive::ArchiveInfo& ResourceArchive::BinaryArchive::loadEntry( Utilities::GUID ID )
//{
//	if ( auto find = loadedEntries.find( ID ); find != loadedEntries.end() )
//		return find->second;
//
//	if ( auto entry = entries.find( ID ); !entry.has_value() )
//		throw ArchiveResourceNotFound( ID );
//	else
//	{
//		auto& info = loadedEntries[ID];
//		info.ID = ID;
//		info.name = entries.get<Entries::Name>( *entry );
//		info.memory.size = entries.get<Entries::DataSize>( *entry );
//
//		stream.seekg( entries.get<Entries::DataStart>( *entry ) );
//		info.memory.data = operator new(info.memory.size);
//		Utilities::Binary::read( stream, info.memory.data, info.memory.size );
//		info.saved = true;
//
//		return info;
//	}
//
//}

