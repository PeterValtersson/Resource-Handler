#ifndef _RESOURCE_HANDLER_H_
#define _RESOURCE_HANDLER_H_
#include <IResourceHandler.h>

namespace Resources
{
	class ResourceHandler : public IResourceHandler {
	public:
		ResourceHandler(std::shared_ptr<MP::IMessageHub> messageHub);
		~ResourceHandler();
	private:
		void _registerResource(MP::Message& message);
		void _loadResource(MP::Message& message);
	};
}
#endif