#include "RZIPArchive.h"
#include <filesystem>
#include <Utilities/Profiler/Profiler.h>
#include <streams/memstream.h>
#include <Utilities/StringUtilities.h>
#include <Utilities/MonadicOptional.h>
#include <Utilities/FStreamHelpers.h>

Utilities::optional<bit7z::BitArchiveItem> find_item( bit7z::BitArchiveInfo& arc, std::wstring name )
{
	auto items = arc.items();
	for ( auto& item : items )
		if ( item.name() == name )
			return item;
	return std::nullopt;
}

Utilities::optional<size_t> find_item_index( bit7z::BitArchiveInfo& arc, std::wstring name )
{
	auto items = arc.items();
	for ( size_t i = 0; i < items.size(); i++ )
		if ( items[i].name() == name )
			return i;
	return std::nullopt;
}

namespace fs = std::filesystem;
Resources::RZIPArchive::RZIPArchive( std::string_view archivePath, AccessMode mode ) : mode( mode ), archive_path( std::string( archivePath ) ), archive_path_w( Utilities::String::utf8_2_utf16( archivePath ) )
{
	bit7z::Bit7zLibrary lib( L"7za.dll" );
	bit7z::BitCompressor comp( lib, bit7z::BitFormat::SevenZip );

	std::vector<bit7z::byte_t> buffer;
	
	if ( fs::exists( archive_path ) )
	{
		try
		{
			bit7z::BitArchiveInfo arc( lib, archive_path_w, bit7z::BitFormat::SevenZip );
			auto aentires = find_item_index( arc, L"entries" );
			bit7z::BitExtractor extractor( lib, bit7z::BitFormat::SevenZip );
			extractor.extract( archive_path_w, buffer, *aentires );
			auto s = Utilities::Binary_Stream::create_stream_from_data( buffer.data(), buffer.size() );
			entries.readFromFile( s.stream );
		}
		catch ( ... )
		{
			throw PathNotAccessible( archivePath );
		}

		archive = ZipFile::Open( archive_path );
		if ( !archive )
			throw PathNotAccessible( archivePath );

		auto aentries = archive->GetEntry( "entries" );
		entries.readFromFile( *aentries->GetDecompressionStream() );
		aentries->CloseDecompressionStream();
	}
	else
	{
		if ( mode != AccessMode::read_write )
			throw PathNotFound( archivePath );

		std::ofstream newArchive( archivePath, std::ios::binary );
		if ( !newArchive.is_open() )
			throw UNKOWN_ERROR;
		newArchive.close();

		archive = ZipFile::Open( archive_path );
		if ( !archive )
			throw UNKOWN_ERROR;
		save_entries();
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
		archive->CreateEntry( std::string( name ) );
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
		archive->CreateEntry( ID.to_string() );
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
		archive->CreateEntry( std::string( name ) );
	}
}

void Resources::RZIPArchive::save( const To_Save& to_save, Utilities::Memory::ChunkyAllocator& allocator )
{
	PROFILE;
	if ( auto find = entries.find( to_save.first ); find.has_value() )
	{
		ZipArchiveEntry::Ptr entry;
		if ( entry = archive->GetEntry( entries.peek<Entries::Name>( *find ) ); !entry )
			entry = archive->CreateEntry( entries.peek<Entries::Name>( *find ) );
		else if ( entry->IsRawStreamOpened() )
			entry->CloseRawStream();
		std::unique_ptr<imemstream> contentStream;
		allocator.peek_data( to_save.second, [&, entry]( const Utilities::Memory::ConstMemoryBlock data )
		{
			contentStream = std::make_unique<imemstream>( (char*)data.get_char(), data.used_size );
			entry->SetCompressionStream( *contentStream.get(), StoreMethod::Create() );
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
			ZipArchiveEntry::Ptr entry;
			if ( entry = archive->GetEntry( entries.peek<Entries::Name>( *find ) ); !entry )
				entry = archive->CreateEntry( entries.peek<Entries::Name>( *find ) );
			else if ( entry->IsRawStreamOpened() )
				entry->CloseRawStream();

			allocator.peek_data( to_save.second, [&, entry]( const Utilities::Memory::ConstMemoryBlock data )
			{
				streams.push_back( std::make_unique<imemstream>( (char*)data.get_char(), data.used_size ) );
				entry->SetCompressionStream( *streams.back().get(), StoreMethod::Create() );
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
	else if ( auto entry = archive->GetEntry( entries.peek<Entries::Name>( *find ) ); entry )
		return entry->GetSize();
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
		auto entry = archive->GetEntry( entries.peek<Entries::Name>( *find ) );
		entry->SetName( std::string( name ) );
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
	if ( auto aentries = archive->GetEntry( "entries" ); !aentries )
		aentries = archive->CreateEntry( "entries" );
	else if ( aentries->IsRawStreamOpened() )
		aentries->CloseRawStream();

	auto aentries = archive->GetEntry( "entries" );
	std::stringstream ss;
	entries.writeToFile( ss );
	aentries->SetCompressionStream( ss );

	ZipFile::Save( archive, archive_path );
}
