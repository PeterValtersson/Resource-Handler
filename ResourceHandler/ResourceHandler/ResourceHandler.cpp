#include "ResourceHandler.h"

Resources::ResourceHandler::ResourceHandler(std::shared_ptr<MP::IMessageHub> messageHub)
	: IResourceHandler(messageHub, 128ms)
{

	AddMessageHandlerPair({ "RegisterResource", std::bind(&ResourceHandler::_registerResource, this, _1) });
	AddMessageHandlerPair({ "LoadResource", std::bind(&ResourceHandler::_loadResource, this, _1) });
}

Resources::ResourceHandler::~ResourceHandler()
{
}

void Resources::ResourceHandler::_registerResource(MP::Message & message)
{
}

void Resources::ResourceHandler::_loadResource(MP::Message & message)
{
}
