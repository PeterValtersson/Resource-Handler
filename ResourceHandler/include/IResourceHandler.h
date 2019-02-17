#ifndef _INTERFACE_RESOURCE_HANDLER_H_
#define _INTERFACE_RESOURCE_HANDLER_H_
#include <memory>
#include "DLL_Export.h"

namespace Resources
{
	
	class Resource_Base;

	class IResourceHandler {
		friend class Resource_Base;
	public:
		virtual ~IResourceHandler( ) { }

		DECLSPEC_RH static std::shared_ptr<IResourceHandler> get( );
	protected:
		IResourceHandler( ) { }

	private:
		virtual void			registerResource( const Resource_Base* resource )noexcept = 0;
		virtual void			refCountInc( const Resource_Base* resource )noexcept = 0;
		virtual void			refCountDec( const Resource_Base* resource )noexcept = 0;
		virtual uint32_t		getRefCount( const Resource_Base* resource )const noexcept = 0;
		virtual Memory_Block	getResourceData( const Resource_Base* resource ) = 0;
	};
}
#endif