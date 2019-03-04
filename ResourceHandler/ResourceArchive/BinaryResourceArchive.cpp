#include "BinaryResourceArchive.h"
#include <filesystem>
#include <Profiler.h>
#include <FStreamHelpers.h>

namespace fs = std::experimental::filesystem;
ResourceArchive::BinaryResourceArchive::BinaryResourceArchive( const std::string & archivePath, ArchiveMode mode ) : mode( mode ), archivePath( archivePath ), allocator( 1024 )
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

ResourceArchive::BinaryResourceArchive::~BinaryResourceArchive()
{
	stream.close();
}

void ResourceArchive::BinaryResourceArchive::readHeader()
{
	stream.seekg( 0 );
	Utilities::Binary::read( stream, header );
}

void ResourceArchive::BinaryResourceArchive::readTail()
{
	stream.seekg( header.tailStart );
	auto result = entries.readFromFile( stream );
	if ( result < 0 )
		throw ArchiveCorrupt( archivePath, result );
}


void ResourceArchive::BinaryResourceArchive::writeHeader()
{
	stream.seekp( 0 );
	Utilities::Binary::write( stream, header );
}

void ResourceArchive::BinaryResourceArchive::writeTail()
{
	stream.seekp( header.tailStart );
	entries.writeToFile( stream );
}

void ResourceArchive::BinaryResourceArchive::save()
{
	if ( mode != ArchiveMode::development )
		throw ArchiveNotInDeveloperMode();

	for ( auto& loadedEntry : loadedEntries )
		if ( !loadedEntry.saved )
		{
			if ( auto findIndex = entries.find( loadedEntry.ID ); !findIndex.has_value() ) // If this is a new entry, add it
			{
				auto index = entries.add( loadedEntry.ID );
				memcpy( entries.get<Entries::Name>( index ), loadedEntry.name.c_str(), loadedEntry.name.size()+1 );
				entries.set<Entries::DataSize>( index, loadedEntry.size );
				entries.set<Entries::DataStart>( index, header.tailStart );
			
				auto inUse = allocator.inUse( loadedEntry.memoryHandle );
				auto memory = allocator.getData( loadedEntry.memoryHandle );

				stream.seekp( header.tailStart );
				Utilities::Binary::write( stream, memory.data, loadedEntry.size );
				header.tailStart += loadedEntry.size;
				if ( !inUse )
					allocator.returnData( loadedEntry.memoryHandle );
					//if (allocator.isValid( loadedEntry.memoryHandle))
			}
			else // Already exists, overwrite
			{
				memcpy( entries.get<Entries::Name>( *findIndex ), loadedEntry.name.c_str(), loadedEntry.name.size() );

				auto inUse = allocator.inUse( loadedEntry.memoryHandle );
				auto memory = allocator.getData( loadedEntry.memoryHandle );

				if ( loadedEntry.size < entries.peek<Entries::DataSize>( *findIndex )) // New size is smaller than old
				{
					header.unusedSpace += entries.get<Entries::DataSize>( *findIndex ) - loadedEntry.size; // update unused space
				}
				else
				{
					header.unusedSpace += entries.get<Entries::DataSize>( *findIndex ); // update unused space
					entries.set<Entries::DataStart>( *findIndex, header.tailStart ); // And set start to tailstart
					header.tailStart += loadedEntry.size; // Move tailstart
				}
				entries.set<Entries::DataSize>( *findIndex, loadedEntry.size );
			

				stream.seekp( entries.peek<Entries::DataStart>( *findIndex ) );
				Utilities::Binary::write( stream, memory.data, loadedEntry.size );

				if ( !inUse ) // If someone else was using this data still.
					allocator.returnData( loadedEntry.memoryHandle );
			}

			loadedEntry.saved = true;
		}
	writeHeader();
	writeTail();
}


bool ResourceArchive::BinaryResourceArchive::exists( Utilities::GUID ID ) const noexcept
{
	if ( auto loadedEntry = guidToEntryIndex.find( ID ); loadedEntry != guidToEntryIndex.end() )
		return true;
	return entries.find( ID ).has_value();
}

size_t ResourceArchive::BinaryResourceArchive::getSize( Utilities::GUID ID ) const
{
	if ( auto loadedEntry = guidToEntryIndex.find( ID ); loadedEntry != guidToEntryIndex.end() )
		return loadedEntries[loadedEntry->second].size;
	else if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ArchiveResourceNotFound( ID );
	else
		return entries.peek<Entries::DataSize>( *entry );
}

std::string ResourceArchive::BinaryResourceArchive::getName( Utilities::GUID ID ) const
{
	if ( auto loadedEntry = guidToEntryIndex.find( ID ); loadedEntry != guidToEntryIndex.end() )
		return loadedEntries[loadedEntry->second].name;
	else if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ArchiveResourceNotFound( ID );
	else
		return entries.peek<Entries::Name>( *entry );
}

void ResourceArchive::BinaryResourceArchive::read( Utilities::GUID ID, const std::function<void( Utilities::Allocators::MemoryBlock )>& callback )
{
	if ( auto loadedEntry = guidToEntryIndex.find( ID ); loadedEntry != guidToEntryIndex.end() )
	{
		auto memory = allocator.getData( loadedEntries[loadedEntry->second].memoryHandle );
		callback( { memory.data, loadedEntries[loadedEntry->second].size } );
		allocator.returnData( loadedEntries[loadedEntry->second].memoryHandle );
	}
	else if ( auto entry = entries.find( ID ); !entry.has_value() )
		throw ArchiveResourceNotFound( ID );
	else
	{
		LoadedEntryInfo info;
		info.ID = ID;
		info.saved = true;
		info.size = entries.get<Entries::DataSize>( *entry );
		info.name = entries.get<Entries::Name>( *entry );
		info.memoryHandle = allocator.allocate( info.size );
		stream.seekg( entries.get<Entries::DataStart>( *entry ) );
		auto memory = allocator.getData( info.memoryHandle );
		memory.size = info.size;
		Utilities::Binary::read( stream, memory.data, memory.size );
		guidToEntryIndex[ID] = loadedEntries.size();
		loadedEntries.push_back( info );
		

		callback( memory );
		allocator.returnData( info.memoryHandle );
	}
}

void ResourceArchive::BinaryResourceArchive::write( Utilities::GUID ID, const void* data, size_t size )
{
	if ( mode != ArchiveMode::development )
		throw ArchiveNotInDeveloperMode( ID );

	if ( size == 0 || data == nullptr )
		throw UNKOWN_ERROR;

	// Already loaded
	if ( auto loadedEntry = guidToEntryIndex.find( ID ); loadedEntry != guidToEntryIndex.end() )
	{
		auto& info = loadedEntries[loadedEntry->second];
		if ( allocator.isValid( info.memoryHandle ) )
		{
			auto memory = allocator.getData( info.memoryHandle );
			if ( size > memory.size )
			{
				auto newHandle = allocator.allocate( size );
				allocator.free( info.memoryHandle );
				info.memoryHandle = newHandle;
				
			}
			memcpy( memory.data, data, size );
			allocator.returnData( info.memoryHandle );
			info.size = size;
			info.saved = false;
			
		}
	}
	else
	{
		LoadedEntryInfo info;
		info.ID = ID;
		info.name = "";
		info.size = size;
		info.saved = false;
		info.memoryHandle = allocator.allocate( size );
		auto memory = allocator.getData( info.memoryHandle );
		memcpy( memory.data, data, info.size );
		allocator.returnData( info.memoryHandle );

		guidToEntryIndex[ID] = loadedEntries.size();	
		loadedEntries.push_back( info );
	}
	//if ( auto find = entries.find( ID ); find.has_value() )
	//	writeEntry( ID, memory, entries.get<Entries::Name>( *find ) );
	//else
	//	writeEntry( ID, memory, "" );
}

void ResourceArchive::BinaryResourceArchive::setName( Utilities::GUID ID, const std::string & name )
{
	if ( mode != ArchiveMode::development )
		throw ArchiveNotInDeveloperMode( ID );

	// Already loaded
	if ( auto loadedEntry = guidToEntryIndex.find( ID ); loadedEntry != guidToEntryIndex.end() )
	{
		auto& info = loadedEntries[loadedEntry->second];
		info.name = name;
		info.saved = false;
	}
	else
	{
		LoadedEntryInfo info;
		info.ID = ID;
		info.name = name;
		info.size = 0;
		info.saved = false;

		guidToEntryIndex[ID] = loadedEntries.size();
		loadedEntries.push_back( info );
	}
}

//
//void ResourceArchive::BinaryResourceArchive::writeEntry( Utilities::GUID ID, Memory_Block memory, std::string_view name )
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
//const ResourceArchive::ArchiveInfo& ResourceArchive::BinaryResourceArchive::loadEntry( Utilities::GUID ID )
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

