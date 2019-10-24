#include <IResourceArchive.h>
#include "BinaryArchive.h"

std::unique_ptr<Resources::IResourceArchive> Resources::IResourceArchive::create_binary_archive( std::string_view path, Resources::AccessMode mode)
{
	return std::make_unique<Resources::BinaryArchive>( path, mode );
}