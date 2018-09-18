#include "ResourceHandler.h"

std::unique_ptr<Resources::IResourceHandler> createResourceHandler(std::shared_ptr<MP::IMessageHub> messageHub)
{
	return std::make_unique<Resources::ResourceHandler>(messageHub);
}
