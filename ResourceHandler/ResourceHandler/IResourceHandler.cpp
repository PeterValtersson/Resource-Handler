#include "ResourceHandler_Read.h"

//#pragma data_seg (".RH_SHAREDMEMORY")
//
//#pragma data_seg() 
//#pragma comment(linker,"/SECTION:.RH_SHAREDMEMORY,RWS")

std::shared_ptr<Resources::IResourceHandler> Resources::IResourceHandler::get()
{
	std::shared_ptr<Resources::IResourceHandler> resourceHandler;
	if ( !resourceHandler )
		throw NoResourceHandler(); //resourceHandler = std::make_shared<ResourceHandler>();
	return resourceHandler;
}

DECLSPEC_RH std::shared_ptr<Resources::IResourceHandler>  Resources::IResourceHandler::create( AccessMode mode, std::shared_ptr<IResourceArchive> archive )
{
	std::shared_ptr<Resources::IResourceHandler> resourceHandler;
	switch ( mode )
	{
	case Resources::AccessMode::read:
		resourceHandler =  std::make_shared<ResourceHandler_Read>( archive );
		break;
	case Resources::AccessMode::read_write:
		resourceHandler = std::make_shared<ResourceHandler_Read>( archive );
		break;
	default:
		throw UNKOWN_ERROR;
		break;
	}
}