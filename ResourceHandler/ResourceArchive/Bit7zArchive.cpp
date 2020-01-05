#include "Bit7zArchive.h"
#include <Utilities/StringUtilities.h>
#include <Utilities/MonadicOptional.h>
#include <filesystem>

namespace fs = std::filesystem;
Utilities::optional<bit7z::BitArchiveItem> find_item( Bit7zArchive::Entries& entries, std::wstring name )
{
	for ( auto& entry : entries )
		if ( entry.name() == name )
			return entry;
	return std::nullopt;
}

Utilities::optional<size_t> find_item_index( Bit7zArchive::Entries& entries, std::wstring name )
{
	for ( size_t i = 0; i < entries.size(); i++ )
		if ( entries[i].name() == name )
			return i;
	return std::nullopt;
}

Bit7zArchive::Ptr Bit7zArchive::create( std::string_view path, const bit7z::BitInOutFormat& format )
{
	return std::move( std::make_shared<Bit7zArchive>( path, format ) );
}

Bit7zArchive::Bit7zArchive( std::string_view path, const bit7z::BitInOutFormat& format )
	: lib( L"7za.dll" )
	, archive_path_w( Utilities::String::utf8_2_utf16( path ) )
	, format( format )
{
	read_archive_info();
}

Utilities::optional<bit7z::BitArchiveItem> Bit7zArchive::get_entry_info( std::string_view name )
{
	if ( auto find = find_item( entries, Utilities::String::utf8_2_utf16( name ) ); !find.has_value() )
		return std::nullopt;
	else
		return *find;
}

void Bit7zArchive::read_entry( std::string_view name, Buffer& out )
{
	if ( auto find = find_item_index( entries, Utilities::String::utf8_2_utf16( name ) ); !find.has_value() )
		throw Resources::ResourceNotFound( Utilities::GUID( name ) );
	else
	{
		bit7z::BitExtractor extractor( lib, format );
		extractor.extract( archive_path_w, out, *find );
	}
}

void Bit7zArchive::create_entry( std::string_view name )
{
	Bit7zArchive::Buffer buffer;
	write_entry( name, buffer );
}

void Bit7zArchive::write_entry( std::string_view name, std::istream& stream )
{
	bit7z::BitStreamCompressor compressor( lib, format );
	compressor.setUpdateMode( true );
	compressor.compress( stream, archive_path_w, Utilities::String::utf8_2_utf16( name ) );
	read_archive_info();
}

void Bit7zArchive::write_entry( std::string_view name, const Buffer& buffer )
{
	bit7z::BitMemCompressor compressor( lib, format );
	compressor.setUpdateMode( true );
	compressor.compress( buffer, archive_path_w, Utilities::String::utf8_2_utf16( name ) );
	read_archive_info();
}

void Bit7zArchive::write_entry( std::string_view name, const Utilities::Memory::ConstMemoryBlock buffer )
{
	auto s = Utilities::Binary_Stream::create_stream_from_data( buffer.get_char(), buffer.used_size );
	write_entry(name, s.stream);
}

void Bit7zArchive::read_archive_info()
{
	if ( fs::exists( archive_path_w ) )
	{
		archive = std::make_unique<bit7z::BitArchiveInfo>( lib, archive_path_w, format );
		entries = archive->items();
	}
}
