#ifndef _INTERFACE_RESOURCE_HANDLER_H_
#define _INTERFACE_RESOURCE_HANDLER_H_
#include <memory>
#include "DLL_Export.h"
#include <IResourceArchive.h>

namespace Resources
{
	struct NoResourceHandler : public Utilities::Exception{
		NoResourceHandler() : Utilities::Exception( "No resource handler has been created." )
		{}
	};

	struct NoResourceData : public Utilities::Exception{
		NoResourceData( Utilities::GUID ID ) : Utilities::Exception( "Resource has no data" + std::to_string( ID ) )
		{}
	};

	using RefCount = uint32_t;
	class IResourceHandler{
		friend class Resource;
	public:
		virtual ~IResourceHandler()
		{}

		DECLSPEC_RH static Resources::IResourceHandler& get();
		DECLSPEC_RH static Resources::IResourceHandler& create( AccessMode mode, std::shared_ptr<IResourceArchive> archive /*, Renderer*/ );
	protected:
		virtual void			register_resource(const Utilities::GUID ID ) = 0;
		virtual void			inc_refCount( const Utilities::GUID ID )noexcept = 0;
		virtual void			dec_refCount( const Utilities::GUID ID )noexcept = 0;
		virtual RefCount		get_refCount( const Utilities::GUID ID )const noexcept = 0;
		virtual void			use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback )const = 0;
		virtual void			write_data( const Utilities::GUID ID, const void* const data, const size_t size )
		{
			throw WriteInReadOnly();
		}
		virtual void			set_type( Utilities::GUID ID, Utilities::GUID type )
		{
			throw WriteInReadOnly();
		}
		virtual void			set_name( Utilities::GUID ID, std::string_view name )
		{
			throw WriteInReadOnly();
		}
		IResourceHandler()
		{}
	};
}
#endif