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

void Resources::BinaryArchive::create( const Utilities::GUID ID )
{
	entries.add( ID, "", 0, 0, 0 );
}

void Resources::BinaryArchive::save_resource_info()
{
	writeHeader();
	writeTail();
}

void Resources::BinaryArchive::save_resource_info_data( const To_Save& to_save, Utilities::Memory::ChunkyAllocator& allocator )
{
	if ( const auto entry = entries.find( to_save.first ); !entry.has_value() )
		throw ResourceNotFound( to_save.first );
	else
	{
		allocator.use_data( to_save.second, [&]( const Utilities::Memory::MemoryBlock mem )
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

void Resources::BinaryArchive::save_resource_info_data( const To_Save_Vector& to_save_vector, Utilities::Memory::ChunkyAllocator& allocator )
{
	PROFILE;
	for ( const auto& to_save : to_save_vector )
		save_resource_info_data( to_save, allocator );
	writeHeader();
	writeTail();
}

void Resources::BinaryArchive::save_resource_info_data( const std::pair<size_t, Utilities::Memory::Handle>& index_handle, Utilities::Memory::ChunkyAllocator& allocator, bool write_info )
{
	allocator.use_data( index_handle.second, [&]( const Utilities::Memory::MemoryBlock mem )
	{
		if ( mem.used_size <= entries.peek<Entries::DataSize>( index_handle.first ) )
		{
			stream.seekp( entries.peek<Entries::DataStart>( index_handle.first ) );
			mem.write_to_stream( stream );
			entries.set<Entries::DataSize>( index_handle.first, mem.used_size );
		}
		else
		{
			stream.seekp( header.tailStart );
			mem.write_to_stream( stream );
			entries.set<Entries::DataStart>( index_handle.first, header.tailStart );
			entries.set<Entries::DataSize>( index_handle.first, mem.used_size );
			header.tailStart = stream.tellp();
		}

	} );
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
	if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ResourceNotFound( ID );
	else
		memcpy( &entries.get<Entries::Name>( *entry ), name.data(), name.size() + 1 );
}

void Resources::BinaryArchive::set_type( const Utilities::GUID ID, const Utilities::GUID type )
{
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
//
//void Resources::BinaryArchive::write( const Utilities::GUID ID, const Utilities::Allocators::MemoryBlock data )
//{
//	PROFILE;
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
//	PROFILE;
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
//		throw ResourceNotFound( ID );
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

