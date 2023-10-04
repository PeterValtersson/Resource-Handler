#ifndef _INTERFACE_RESOURCE_HANDLER_H_
#define _INTERFACE_RESOURCE_HANDLER_H_
#include <memory>
#include "DLL_Export.h"
#include <IResourceArchive.h>
#include <Utilities/Flags.h>
#include <string>
#include "LoaderSignatures.h"

namespace ResourceHandler
{
	enum class Flags : uint32_t
	{
		None,
		Persistent = 1 << 0,
		RuntimeResource = 1 << 1,
		Modifying = 1 << 2,
		All = Persistent | RuntimeResource | Modifying
	};
	ENUM_FLAGS(Flags);

	enum class Status : uint32_t
	{
		None,
		NotFound = 1 << 0,
		CouldNotLoad = 1 << 1,
		InMemory = 1 << 2,
		Loading = 1 << 3,
		Parsing = 1 << 4,
		Parsed = 1 << 5,
		All = NotFound | CouldNotLoad | InMemory | Loading | Parsing | Parsed
	};
	ENUM_FLAGS(Status);

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
		DECLSPEC_RH static std::shared_ptr<ResourceHandler::IResourceHandler> create(AccessMode mode, std::shared_ptr<IResourceArchive> archive);

		virtual void set_parser_memory_handler(std::shared_ptr<Utilities::Memory::Allocator> memory_handler) = 0;
		virtual std::shared_ptr<IResourceArchive> get_archive() = 0;
		virtual void add_parser(const Utilities::GUID type, const std::string& library_path) = 0;
		virtual void add_parser(const Utilities::GUID type, const parse_callback& parse_callback) = 0;

		virtual void save_all()
		{
			throw WriteInReadOnly();
		}

	protected:
		/* Only called by Resource*/
		virtual void		register_resource(const Utilities::GUID ID, const Flags flag = Flags::None)noexcept = 0;
		virtual Status		get_status(const Utilities::GUID ID)noexcept = 0;
		virtual void		set_flag(const Utilities::GUID ID, const Flags flag)noexcept = 0;
		virtual void		remove_flag(const Utilities::GUID ID, const Flags flag)noexcept = 0;
		virtual void		inc_refCount( const Utilities::GUID ID )noexcept = 0;
		virtual void		dec_refCount( const Utilities::GUID ID )noexcept = 0;
		virtual RefCount	get_refCount( const Utilities::GUID ID )const noexcept = 0;
		virtual std::string	get_name( const Utilities::GUID ID )const  = 0;
		virtual void		use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback ) = 0;
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
#endif