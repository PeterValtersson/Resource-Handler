#include "ResourceHandler_Read.h"

#pragma data_seg (".RH_SHAREDMEMORY")
std::shared_ptr<Resources::IResourceHandler> resourceHandler = nullptr;
#pragma data_seg() 
#pragma comment(linker,"/SECTION:.RH_SHAREDMEMORY,RWS")

std::shared_ptr<Resources::IResourceHandler> Resources::IResourceHandler::get()
{
	if ( !resourceHandler )
		throw NoResourceHandler(); //resourceHandler = std::make_shared<ResourceHandler>();
	return resourceHandler;
}

DECLSPEC_RH std::shared_ptr<Resources::IResourceHandler> Resources::IResourceHandler::create( AccessMode mode, std::shared_ptr<IResourceArchive> archive )
{
	switch ( mode )
	{
	case Resources::AccessMode::read_only:
		resourceHandler =  std::make_shared<ResourceHandler_Read>( archive );
	case Resources::AccessMode::read_write:
		resourceHandler = std::make_shared<ResourceHandler_Read>( archive );
		break;
	default:
		throw UNKOWN_ERROR;
		break;
	}
	return resourceHandler;
}