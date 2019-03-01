#include <IResourceArchive.h>
#include "BinaryResourceArchive.h"

std::unique_ptr<ResourceArchive::IResourceArchive> ResourceArchive::createResourceArchive( const std::string& path, ArchiveMode mode)
{
	return std::make_unique<BinaryResourceArchive>( path, mode );
}

ResourceArchive::IResourceArchive::IResourceArchive()
{ }


ResourceArchive::IResourceArchive::~IResourceArchive()
{ }
