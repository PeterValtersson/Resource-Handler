#include "ResourceHandler_Read.h"
#include "ResourceHandler_Write.h"

#pragma data_seg (".RH_SHAREDMEMORY")
std::weak_ptr<Resources::IResourceHandler> resource_handler;
#pragma data_seg() 
#pragma comment(linker,"/SECTION:.RH_SHAREDMEMORY,RWS")
std::shared_ptr<Resources::IResourceHandler> Resources::IResourceHandler::get()
{
	if (auto spt = resource_handler.lock())
		return spt;
	else
		throw NoResourceHandler();
}

DECLSPEC_RH void Resources::IResourceHandler::set(std::shared_ptr<Resources::IResourceHandler> rh)
{
	resource_handler = rh;
}

DECLSPEC_RH std::shared_ptr<Resources::IResourceHandler> Resources::IResourceHandler::create( AccessMode mode, std::shared_ptr<IResourceArchive> archive )
{
	switch ( mode )
	{
	case Resources::AccessMode::read:
		return std::make_shared<ResourceHandler_Read>( archive );
		break;
	case Resources::AccessMode::read_write:
		return std::make_shared<ResourceHandler_Write>( archive );
		break;
	default:
		throw UNKOWN_ERROR;
		break;
	}
}