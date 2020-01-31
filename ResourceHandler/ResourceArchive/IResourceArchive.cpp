#include <IResourceArchive.h>
#include "BinaryArchive.h"
//#include "RZIPArchive.h"

std::shared_ptr<ResourceHandler::IResourceArchive> ResourceHandler::IResourceArchive::create_binary_archive( std::string_view path, ResourceHandler::AccessMode mode)
{
	return std::make_shared<ResourceHandler::BinaryArchive>( path, mode );
}

std::shared_ptr<ResourceHandler::IResourceArchive> ResourceHandler::IResourceArchive::create_zip_archive( std::string_view path, AccessMode mode )
{
	return std::make_shared<ResourceHandler::BinaryArchive>( path, mode );
	//return std::make_shared<ResourceHandler::RZIPArchive>( path, mode );
}
