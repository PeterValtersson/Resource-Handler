#include "BinaryResourceArchive.h"



ResourceArchive::BinaryResourceArchive::BinaryResourceArchive( const std::string& archivePath, ArchiveMode mode ) : _mode( mode )
{
	if ( mode == ArchiveMode::development )
		_archiveStream.open( archivePath, std::ios::binary | std::ios::in | std::ios::out );
	else
		_archiveStream.open( archivePath, std::ios::binary | std::ios::in );

	if ( !_archiveStream.is_open() )
		throw ArchivePathNotFound( archivePath );


}


ResourceArchive::BinaryResourceArchive::~BinaryResourceArchive()
{
	_archiveStream.close();
}

bool ResourceArchive::BinaryResourceArchive::exists( Utilities::GUID ID ) const noexcept
{
	return _archive.header.find( ID ).has_value();
}

size_t ResourceArchive::BinaryResourceArchive::size( Utilities::GUID ID ) const
{
	return _archive.header.find( ID ).or_else( [&]
	{
		throw ArchiveResourceNotFound( ID );
	} ).map( [&]( size_t index )
	{
		return _archive.header.peek<Archive::Header::dataSize>( index );
	} ).value();
}

std::string_view ResourceArchive::BinaryResourceArchive::name( Utilities::GUID ID ) const
{
	return  _archive.header.find( ID ).or_else( [&]
	{
		throw ArchiveResourceNotFound( ID );
	} ).map( [&]( size_t index )
	{
		return _archive.header.peek<Archive::Header::name>( index );
	} ).value();
}

ResourceArchive::ArchiveInfo ResourceArchive::BinaryResourceArchive::read( Utilities::GUID ID )
{
	return _archive.header.find( ID ).or_else( [&]
	{
		throw ArchiveResourceNotFound( ID );
	} ).map( [&]( size_t index )
	{
		return _archive.loadArchiveResource( index );
	} ).value();
}

void ResourceArchive::BinaryResourceArchive::write( Utilities::GUID ID, Memory_Block & memory )
{ }


ResourceArchive::ArchiveInfo ResourceArchive::BinaryResourceArchive::Archive::loadArchiveResource( std::size_t index )
{
	return ArchiveInfo();
}
