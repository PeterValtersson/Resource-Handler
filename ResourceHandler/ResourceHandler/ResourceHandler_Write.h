#ifndef _RESOURCE_HANDLER_WRITE_H_
#define _RESOURCE_HANDLER_WRITE_H_
#pragma once
#include <IResourceHandler.h>
#include <Utilities/Memory/Sofa.h>
#include <Utilities/Memory/ChunkyAllocator.h>
namespace ResourceHandler
{
	class ResourceHandler_Write : public IResourceHandler
	{
	public:
		ResourceHandler_Write( std::shared_ptr<IResourceArchive> archive );
		~ResourceHandler_Write();

		virtual void save_all()override;
	protected:
		virtual void			register_resource( const Utilities::GUID ID )noexcept override;
		virtual Status			get_status( const Utilities::GUID ID )noexcept  override;
		virtual void			inc_refCount( const Utilities::GUID ID )noexcept  override;
		virtual void			dec_refCount( const Utilities::GUID ID )noexcept  override;
		virtual RefCount		get_refCount( const Utilities::GUID ID )const noexcept override;
		virtual std::string		get_name( const Utilities::GUID ID )const  override;
		virtual void			use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback ) override;

		virtual void			modify_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::MemoryBlock )>& callback ) override;
		virtual void		write_data( Utilities::GUID ID, const void* const data, const size_t size );
		virtual void		set_type( Utilities::GUID ID, Utilities::GUID type );
		virtual void		set_name( Utilities::GUID ID, std::string_view name );

	private:
		struct Entries : public Utilities::Memory::SofA<Utilities::GUID, Utilities::GUID::Hasher,
			Status, // Status,
			Utilities::Memory::Handle,	 // Raw
			Utilities::Memory::Handle,	 // Parsed / VRAM info(Vertex Count, etc.)
			RefCount,
			bool						// HasChanged
		> {
			enum {
				ID,
				Status,
				Memory_Raw,
				Memory_Parsed,
				RefCount,
				HasChanged
			};
		} resources;

		std::shared_ptr<IResourceArchive> archive;
		Utilities::Memory::ChunkyAllocator allocator;
	};
}


#endif