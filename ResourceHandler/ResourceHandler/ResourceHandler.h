#ifndef _RESOURCE_HANDLER_H_
#define _RESOURCE_HANDLER_H_
#include <IResourceHandler.h>
#include <CircularFIFO.h>
#include <thread>
#include <Sofa.h>
#include <Flags.h>
#include <ChunkyAllocator.h>
#include <future>

namespace Resources {
	enum class Resource_State {
		Pass0 = 1 << 0,
		Pass1 = 1 << 1,
		Pass2 = 1 << 2
	};
	

	class ResourceHandler : public IResourceHandler {
	public:
		ResourceHandler( std::vector<std::unique_ptr<IResourceArchive>>& archives );
		~ResourceHandler();

		virtual void			update()noexcept override;
	protected:
		virtual void			register_resource( Utilities::GUID ID )noexcept override;
		virtual void			inc_refCount( Utilities::GUID ID )  override;
		virtual void			dec_refCount( Utilities::GUID ID )  override;
		virtual RefCount		get_refCount( Utilities::GUID ID )const override;
		virtual void			use_data( Utilities::GUID ID, const std::function<void( const Utilities::Allocators::MemoryBlock )>& callback ) override;

		struct Entries : public Utilities::Sofa::Array::Sofa<Utilities::GUID, Utilities::GUID::Hasher,
			Utilities::Allocators::Handle,	 // Data Handle
			RefCount, // RefCount
			Resource_State // State
		> {
			static const uint8_t ID = 0;
			static const uint8_t Handle = 1;
			static const uint8_t RefCount = 2;
			static const uint8_t State = 3;
		} resources;
	protected:
		std::vector<std::unique_ptr<IResourceArchive>> archives;
		Utilities::Allocators::ChunkyAllocator allocator;
		std::vector<std::string> log;
		bool running;

		struct Pass0 {
			Utilities::CircularFiFo<std::pair<Utilities::GUID, std::promise<Utilities::Allocators::Handle>>> to_load;
			std::vector<std::pair<Utilities::GUID, std::future<Utilities::Allocators::Handle>>> futures;
			bool loading( Utilities::GUID ID )
			{
				for ( auto& p : futures )
					if ( p.first == ID )
						return true;
				return false;
			}
			std::thread thread;
			void entry( bool* running, ResourceHandler* rh );


		}pass0;


		Utilities::CircularFiFo<Utilities::Allocators::Handle> loaded_pass1;
	};
}
ENUM_FLAGS( Resources::Resource_State );
#endif