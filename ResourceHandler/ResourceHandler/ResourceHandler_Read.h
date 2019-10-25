#ifndef _RESOURCE_HANDLER_H_
#define _RESOURCE_HANDLER_H_
#include <IResourceHandler.h>
#include <Utilities/CircularFIFO.h>
#include <thread>
#include <Utilities/Memory/Sofa.h>
#include <Utilities/Flags.h>
#include <Utilities/Memory/ChunkyAllocator.h>
#include <future>
#include <Utilities/Concurrent.h>

namespace Resources
{
	enum class Resource_State{
		Pass0 = 1 << 0,
		Pass1 = 1 << 1,
		Pass2 = 1 << 2
	};


	class ResourceHandler_Read : public IResourceHandler{
	public:
		ResourceHandler_Read( std::shared_ptr<IResourceArchive> archive );
		~ResourceHandler_Read();

		
	protected:
		virtual void			register_resource( Utilities::GUID ID ) override;
		virtual void			inc_refCount( Utilities::GUID ID )noexcept  override;
		virtual void			dec_refCount( Utilities::GUID ID )noexcept  override;
		virtual RefCount		get_refCount( Utilities::GUID ID )const noexcept override;
		virtual void			use_data( Utilities::GUID ID, const std::function<void( const Utilities::Memory::MemoryBlock )>& callback ) override;

		struct Entries : public Utilities::Memory::SofA<Utilities::GUID, Utilities::GUID::Hasher,
			Resource_State, // State,
			Utilities::Memory::Handle,	 // pass0Handle
			Utilities::Memory::Handle,	 // pass1Handle
			Utilities::Memory::Handle,	 // pass2Handle
			RefCount, // RefCount
			
		>{
			static const uint8_t ID = 0;
			static const uint8_t State = 1;
			static const uint8_t pass0Handle = 2;
			static const uint8_t pass1Handle = 3;
			static const uint8_t pass2Handle = 4;
			static const uint8_t RefCount = 5;	
		} resources;
	protected:
		virtual void			update()noexcept;

		std::shared_ptr<IResourceArchive> archive;
		Utilities::Concurrent<Utilities::Memory::ChunkyAllocator> allocator;
		std::thread thread;
		std::vector<std::string> log;
		bool running;

		struct Pass0{
			Utilities::CircularFiFo<std::pair<Utilities::GUID, std::promise<Utilities::Memory::Handle>>> to_load;
			std::vector<std::pair<Utilities::GUID, std::future<Utilities::Memory::Handle>>> futures;
			bool loading( Utilities::GUID ID )
			{
				for ( auto& p : futures )
					if ( p.first == ID )
						return true;
				return false;
			}
			std::thread thread;
			void entry( bool* running )noexcept;


			Pass0( std::shared_ptr<IResourceArchive> archive,
				   Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator ) : archive( archive ), allocator( allocator )
			{}
			std::shared_ptr<IResourceArchive> archive;
			Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator;
		}pass0;


		Utilities::CircularFiFo<Utilities::Memory::Handle> loaded_pass1;
	};
}
ENUM_FLAGS( Resources::Resource_State );
#endif