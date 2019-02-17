#include "ResourceHandler.h"

#pragma data_seg (".RH_SHAREDMEMORY")
std::shared_ptr<Resources::IResourceHandler> resourceHandler = nullptr;
#pragma data_seg() 
#pragma comment(linker,"/SECTION:.RH_SHAREDMEMORY,RWS")

std::shared_ptr<Resources::IResourceHandler> Resources::IResourceHandler::get()
{
	if ( !resourceHandler )
		resourceHandler = std::make_shared<ResourceHandler>();
	return resourceHandler;
}
