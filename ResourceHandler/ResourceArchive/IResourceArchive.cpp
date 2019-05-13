#include <IResourceArchive.h>
#include "BinaryArchive.h"

std::unique_ptr<Resources::IResourceArchive> Resources::createResourceArchive( const std::string& path, Resources::AccessMode mode)
{
	return std::make_unique<Resources::BinaryArchive>( path, mode );
}