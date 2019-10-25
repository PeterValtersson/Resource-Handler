#ifndef _INTERFACE_RESOURCE_HANDLER_H_
#define _INTERFACE_RESOURCE_HANDLER_H_
#include <memory>
#include "DLL_Export.h"
#include <IResourceArchive.h>

namespace Resources {
	struct NoResourceHandler : public Utilities::Exception {
		NoResourceHandler() : Utilities::Exception( "No resource handler has been created." ) { }
	};
	
	using RefCount = uint32_t;
	class IResourceHandler {
		friend class Resource;
	public:
		virtual ~IResourceHandler() { }

		DECLSPEC_RH static std::shared_ptr<IResourceHandler> get();
		DECLSPEC_RH static std::shared_ptr<IResourceHandler> create( AccessMode mode, std::shared_ptr<IResourceArchive> archive /*, Renderer*/ );	
	protected:
		virtual void			register_resource( Utilities::GUID ID ) = 0;
		virtual void			inc_refCount( Utilities::GUID ID )noexcept = 0;
		virtual void			dec_refCount( Utilities::GUID ID )noexcept = 0;
		virtual RefCount		get_refCount( Utilities::GUID ID )const noexcept = 0;
		virtual void			use_data( Utilities::GUID ID, const std::function<void( const Utilities::Memory::MemoryBlock )>& callback ) = 0;
		virtual void			write_data( Utilities::GUID ID, const Utilities::Memory::MemoryBlock data ) { throw WriteInReadOnly(); }
		virtual void			set_type( Utilities::GUID ID ) { throw WriteInReadOnly(); }
		virtual void			set_name( Utilities::GUID ID, const std::string& name ) { throw WriteInReadOnly(); }
		IResourceHandler() { }
	};
}
#endif