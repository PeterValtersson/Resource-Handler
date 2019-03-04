#ifndef _INTERFACE_RESOURCE_HANDLER_H_
#define _INTERFACE_RESOURCE_HANDLER_H_
#include <memory>
#include "DLL_Export.h"
#include <IResourceArchive.h>

namespace Resources
{
	class IResourceHandler {
		friend class Resource_Base;
	public:
		virtual ~IResourceHandler( ) { }

		DECLSPEC_RH static std::shared_ptr<IResourceHandler> get( );
	protected:
		IResourceHandler( ) { }

	private:
		virtual void			registerResource( Utilities::GUID ID ) = 0;
		virtual void			checkIn( Utilities::GUID ID )= 0;
		virtual void			checkOut( Utilities::GUID ID ) = 0;
		virtual uint32_t		getRefCount( Utilities::GUID ID )const  = 0;
		virtual ResourceArchive::ArchiveEntry	getResourceData( Utilities::GUID ID ) = 0;
	};
}
#endif