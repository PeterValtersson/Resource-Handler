#ifndef _BIT_7Z_ARCHIVE_H_
#define _BIT_7Z_ARCHIVE_H_
#pragma once
#include <bit7z.hpp>
#include <memory> 
#include <IResourceArchive.h>
#include <Utilities/Memory/MemoryBlock.h>
#include <Utilities/MonadicOptional.h>

class Bit7zArchive {
public:
	using Ptr = std::shared_ptr<Bit7zArchive>;
	using Entries = std::vector<bit7z::BitArchiveItem>;
	using Buffer = std::vector<bit7z::byte_t>;
	using Archive = std::unique_ptr<bit7z::BitArchiveInfo>;
	static Ptr create( std::string_view path, const bit7z::BitInOutFormat& format = bit7z::BitFormat::SevenZip );
	
	Bit7zArchive( std::string_view path, const bit7z::BitInOutFormat& format = bit7z::BitFormat::SevenZip );


	Utilities::optional<bit7z::BitArchiveItem> get_entry_info( std::string_view name );
	void read_entry( std::string_view name, Buffer& out );
	void create_entry( std::string_view name );
	void write_entry( std::string_view name, std::istream& stream );
	void write_entry( std::string_view name, const Buffer& buffer );
	void write_entry( std::string_view name, const Utilities::Memory::ConstMemoryBlock buffer );

private:
	void read_archive_info();
private:
	bit7z::Bit7zLibrary lib;
	std::wstring archive_path_w;
	const bit7z::BitInOutFormat& format;
	Archive archive;
	Entries entries;
};

#endif