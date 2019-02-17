#ifndef _RESOURCE_HANDLER_H_
#define _RESOURCE_HANDLER_H_
#include <IResourceHandler.h>

namespace Resources
{
	class ResourceHandler : public IResourceHandler {
	public:
		ResourceHandler();
		~ResourceHandler();
	};
}
#endif