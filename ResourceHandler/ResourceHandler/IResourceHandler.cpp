#include "ResourceHandler_Read.h"

#pragma data_seg (".RH_SHAREDMEMORY")
std::unique_ptr<Resources::IResourceHandler> resource_handler;
#pragma data_seg() 
#pragma comment(linker,"/SECTION:.RH_SHAREDMEMORY,RWS")
Resources::IResourceHandler& Resources::IResourceHandler::get()
{
	if ( !resource_handler )
		throw NoResourceHandler();
	return *resource_handler;
}

DECLSPEC_RH Resources::IResourceHandler& Resources::IResourceHandler::create( AccessMode mode, std::shared_ptr<IResourceArchive> archive )
{
	switch ( mode )
	{
	case Resources::AccessMode::read:
		resource_handler = std::make_unique<ResourceHandler_Read>( archive );
		break;
	case Resources::AccessMode::read_write:
		resource_handler = std::make_unique<ResourceHandler_Read>( archive );
		break;
	default:
		throw UNKOWN_ERROR;
		break;
	}
	return *resource_handler;
}