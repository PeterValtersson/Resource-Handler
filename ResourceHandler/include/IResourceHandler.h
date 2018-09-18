#ifndef _INTERFACE_RESOURCE_HANDLER_H_
#define _INTERFACE_RESOURCE_HANDLER_H_
#include <memory>
#include <Client.h>
#include "DLL_Export.h"

namespace Resources
{
	class IResourceHandler : public MP::Client {
	public:
		virtual ~IResourceHandler() {}
		virtual const Utilities::GUID Identifier()const noexcept final
		{
			return "ResourceHandler"_hash;
		}
	protected:
		IResourceHandler(std::shared_ptr<MP::IMessageHub> messageHub, std::chrono::milliseconds timePerFrame)
			: MP::Client(messageHub, timePerFrame) {}
	};

	DECLSPEC std::unique_ptr<IResourceHandler> createResourceHandler(std::shared_ptr<MP::IMessageHub> messageHub);

}
#endif