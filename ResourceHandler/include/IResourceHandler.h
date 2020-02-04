#ifndef _INTERFACE_RESOURCE_HANDLER_H_
#define _INTERFACE_RESOURCE_HANDLER_H_
#include <memory>
#include "DLL_Export.h"
#include <IResourceArchive.h>
#include <Utilities/Flags.h>

namespace ResourceHandler
{
	enum class Flags{
		None,
		Persistent
	};

	ENUM_FLAGS( ResourceHandler::Flags );

	enum class Status : uint32_t {
		None = 0 << 0,
		Not_Found = 1 << 1,
		Could_Not_Load = 1 << 2,
		In_Memory = 1 << 3,
		Could_Not_Parse = 1 << 4,
		In_Memory_Parsed = 1 << 5,
		Loading = 1 << 6,
		Parsing = 1 << 7
	};

	enum class Memory_Type
	{
		RAM,
		VRAM
	};
	struct NoResourceHandler : public Utilities::Exception{
		NoResourceHandler() : Utilities::Exception( "No resource handler has been created." )
		{}
	};

	struct NoResourceData : public Utilities::Exception {
		NoResourceData( const std::string& name, Utilities::GUID ID ) : Utilities::Exception( "Resource has no data: " + name + ", GUID:" + std::to_string( ID ) )
		{}
	};

	typedef uint32_t RefCount;
	class IResourceHandler{
		friend class Resource;
	public:
		virtual ~IResourceHandler()
		{}

		DECLSPEC_RH static std::shared_ptr<ResourceHandler::IResourceHandler> get();
		DECLSPEC_RH static void set(std::shared_ptr<ResourceHandler::IResourceHandler> rh);
		DECLSPEC_RH static std::shared_ptr<ResourceHandler::IResourceHandler> create( AccessMode mode, std::shared_ptr<IResourceArchive> archive );

		virtual void save_all()
		{
			throw WriteInReadOnly();
		}

	protected:
		/* Only called by Resource*/
		virtual void		register_resource(const Utilities::GUID ID )noexcept = 0;
		virtual void		inc_refCount( const Utilities::GUID ID )noexcept = 0;
		virtual void		dec_refCount( const Utilities::GUID ID )noexcept = 0;
		virtual RefCount	get_refCount( const Utilities::GUID ID )const noexcept = 0;
		virtual std::string	get_name( const Utilities::GUID ID )const  = 0;
		virtual void		use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback ) noexcept = 0;
		virtual void		modify_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::MemoryBlock )>& callback )
		{
			throw WriteInReadOnly();
		}
		virtual void		write_data( const Utilities::GUID ID, const void* const data, const size_t size )
		{
			throw WriteInReadOnly();
		}
		virtual void		set_type( Utilities::GUID ID, Utilities::GUID type )
		{
			throw WriteInReadOnly();
		}
		virtual void		set_name( Utilities::GUID ID, std::string_view name )
		{
			throw WriteInReadOnly();
		}
		IResourceHandler()
		{}
	};
}
ENUM_FLAGS(ResourceHandler::Status);
#endif