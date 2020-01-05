#include <IResourceArchive.h>
#include "BinaryArchive.h"
//#include "RZIPArchive.h"

std::shared_ptr<Resources::IResourceArchive> Resources::IResourceArchive::create_binary_archive( std::string_view path, Resources::AccessMode mode)
{
	return std::make_shared<Resources::BinaryArchive>( path, mode );
}

DECLSPEC_RA std::shared_ptr<Resources::IResourceArchive> Resources::IResourceArchive::create_zip_archive( std::string_view path, AccessMode mode )
{
	return std::make_shared<Resources::BinaryArchive>( path, mode );
	//return std::make_shared<Resources::RZIPArchive>( path, mode );
}
