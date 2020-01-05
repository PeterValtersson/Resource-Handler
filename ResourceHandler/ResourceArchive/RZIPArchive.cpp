#include "RZIPArchive.h"
#include <filesystem>
#include <Utilities/Profiler/Profiler.h>
#include <streams/memstream.h>
#include <Utilities/FStreamHelpers.h>


namespace fs = std::filesystem;
Resources::RZIPArchive::RZIPArchive( std::string_view archivePath, AccessMode mode ) : mode( mode ), archive_path( std::string( archivePath ) )
{
	bit7z::Bit7zLibrary lib( L"7za.dll" );

	
	
	if ( fs::exists( archive_path ) )
	{
		Utilities::optional<size_t> aentires;
		try
		{
			archive = Bit7zArchive::create( archivePath );
		}
		catch ( ... )
		{
			throw PathNotAccessible( archivePath );
		}

		try
		{
			std::vector<bit7z::byte_t> buffer;
			archive->read_entry("Entries", buffer);
			auto s = Utilities::Binary_Stream::create_stream_from_data( buffer.data(), buffer.size() );
			entries.readFromFile( s.stream );
		}
		catch ( ... )
		{
			throw UNKOWN_ERROR;
		}
	}
	else
	{
		if ( mode != AccessMode::read_write )
			throw PathNotFound( archivePath );
		try
		{
			archive = Bit7zArchive::create( archivePath );
			std::stringstream ss;
			entries.writeToFile( ss );
			ss.seekg( 0 );
			archive->write_entry( "Entries", ss );
		}
		catch ( ... )
		{
			throw UNKOWN_ERROR;
		}
	}
}

Resources::RZIPArchive::~RZIPArchive()
{

}

const size_t Resources::RZIPArchive::num_resources() const noexcept
{
	return entries.size();
}

Utilities::GUID Resources::RZIPArchive::create_from_name( std::string_view name )
{
	PROFILE;
	if ( auto find = entries.find( Utilities::GUID( name ) ); find.has_value() )
		throw ResourceExists( name, Utilities::GUID( name ) );
	else
	{
		entries.add( Utilities::GUID( name ), name );
		archive->create_entry( name );
	}
	return Utilities::GUID( name );
}

void Resources::RZIPArchive::create_from_ID( const Utilities::GUID ID )
{
	PROFILE;
	if ( auto find = entries.find( ID ); find.has_value() )
		throw ResourceExists( ID.to_string(), ID );
	else
	{
		entries.add( ID, ID.to_string() );
		archive->create_entry( ID.to_string() );
	}
}

void Resources::RZIPArchive::create( const Utilities::GUID ID, std::string_view name )
{
	PROFILE;
	if ( auto find = entries.find( ID ); find.has_value() )
		throw ResourceExists( name, ID );
	else
	{
		entries.add( ID, name );
		archive->create_entry( name );
	}
}

void Resources::RZIPArchive::save( const To_Save& to_save, Utilities::Memory::ChunkyAllocator& allocator )
{
	PROFILE;
	if ( auto find = entries.find( to_save.first ); find.has_value() )
	{
		allocator.peek_data( to_save.second, [&, this]( const Utilities::Memory::ConstMemoryBlock data )
		{
			archive->write_entry( entries.peek<Entries::Name>( *find ), data );
		} );

		save_entries();
	}
}

void Resources::RZIPArchive::save_multiple( const To_Save_Vector& to_save_vector, Utilities::Memory::ChunkyAllocator& allocator )
{
	PROFILE;
	std::vector<std::unique_ptr<imemstream>> streams;
	for ( auto& to_save : to_save_vector )
	{
		if ( auto find = entries.find( to_save.first ); find.has_value() )
		{
			allocator.peek_data( to_save.second, [&, this]( const Utilities::Memory::ConstMemoryBlock data )
			{
				archive->write_entry( entries.peek<Entries::Name>( *find ), data );
			} );
		}
	}

	save_entries();
}

const bool Resources::RZIPArchive::exists( const Utilities::GUID ID ) const noexcept
{
	return entries.find( ID ).has_value();
}

const size_t Resources::RZIPArchive::get_size( const Utilities::GUID ID ) const
{
	PROFILE;
	if ( auto find = entries.find( ID ); !find.has_value() )
		throw ResourceNotFound( ID );
	else if ( auto entry = archive->get_entry_info( entries.peek<Entries::Name>( *find ) ); entry.has_value() )
		return entry->size();
	else
		return 0;
}

const std::string Resources::RZIPArchive::get_name( const Utilities::GUID ID ) const
{
	PROFILE;
	if ( auto find = entries.find( ID ); !find.has_value() )
		throw ResourceNotFound( ID );
	else
		return entries.peek<Entries::Name>( *find );
}

const Utilities::GUID Resources::RZIPArchive::get_type( const Utilities::GUID ID ) const
{
	return Utilities::GUID();
}

void Resources::RZIPArchive::set_name( const Utilities::GUID ID, std::string_view name )
{
	PROFILE;
	if ( auto find = entries.find( ID ); !find.has_value() )
	{
		if ( mode == AccessMode::read )
			throw ResourceNotFound( ID );
		else
		{
			create( ID, name );
		}
	}

	else
	{
		// Set name of file as well
		memcpy( &entries.get<Entries::Name>( *find ), name.data(), name.size() + 1 );
	}
	save_entries();
}

void Resources::RZIPArchive::set_type( const Utilities::GUID ID, const Utilities::GUID type )
{}

const Utilities::Memory::Handle Resources::RZIPArchive::read( const Utilities::GUID ID, Utilities::Memory::ChunkyAllocator& allocator )
{
	PROFILE;
	if ( auto find = entries.find( ID ); !find.has_value() )
		throw ResourceNotFound( ID );
	else
	{
		auto entry = archive->GetEntry( entries.peek<Entries::Name>( *find ) );
		auto handle = allocator.allocate( entry->GetSize() );
		allocator.use_data( handle, [entry]( Utilities::Memory::MemoryBlock data )
		{
			data.read_from_stream( *entry->GetDecompressionStream() );
		} );
		if ( entry->IsDecompressionStreamOpened() )
			entry->CloseDecompressionStream();
		return handle;
	}
	return 0;
}

void Resources::RZIPArchive::save_entries() noexcept
{
	if ( auto aentries = archive->GetEntry( "Entries" ); !aentries )
		aentries = archive->CreateEntry( "Entries" );
	else if ( aentries->IsRawStreamOpened() )
		aentries->CloseRawStream();

	auto aentries = archive->GetEntry( "Entries" );
	std::stringstream ss;
	entries.writeToFile( ss );
	aentries->SetCompressionStream( ss );

	ZipFile::Save( archive, archive_path );
}
