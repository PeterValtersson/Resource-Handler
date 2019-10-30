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
	enum class Pass{
		Unloaded = 0,
		Loaded_Raw = 1 << 0,
		Loaded_Parsed = 1 << 1,
		Loaded_VRAM = 1 << 2
	};

	class ResourceHandler_Read : public IResourceHandler{
	public:
		ResourceHandler_Read( std::shared_ptr<IResourceArchive> archive );
		~ResourceHandler_Read();


	protected:
		virtual void			register_resource( const Utilities::GUID ID ) override;
		virtual void			inc_refCount( const Utilities::GUID ID )noexcept  override;
		virtual void			dec_refCount( const Utilities::GUID ID )noexcept  override;
		virtual RefCount		get_refCount( const Utilities::GUID ID )const noexcept override;
		virtual void			use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback )const override;
	
		struct use_data_info{
			Utilities::GUID ID;
			std::promise<Utilities::Memory::Handle> promise;
		};
		mutable Utilities::CircularFiFo<use_data_info>	use_data_queue;
		void											use_datas()const noexcept;



		struct Entries : public Utilities::Memory::SofA<Utilities::GUID, Utilities::GUID::Hasher,
			Pass, // Passes loaded,
			Utilities::Memory::Handle,	 // pass0Handle
			Utilities::Memory::Handle,	 // pass1Handle
			Utilities::Memory::Handle,	 // pass2Handle
			RefCount
		>{
			static const uint8_t ID = 0;
			static const uint8_t passes_loaded = 1;
			static const uint8_t raw_handle = 2;
			static const uint8_t parsed_handle = 3;
			static const uint8_t vram_handle = 4;
			static const uint8_t ref_count = 5;
		} ;
		Utilities::Concurrent<Entries> resources;

		virtual void			update()noexcept;
		virtual void			send_resouces_for_raw_loading()noexcept;
		virtual void			process_resouces_from_raw_loading()noexcept;

		std::shared_ptr<IResourceArchive> archive;
		Utilities::Concurrent<Utilities::Memory::ChunkyAllocator> allocator;
		std::thread thread;
		std::vector<std::string> log;
		bool running;

		struct Loader_Raw{
			Utilities::CircularFiFo<std::pair<Utilities::GUID, std::promise<Utilities::Memory::Handle>>> to_load;
			std::vector<std::pair<Utilities::GUID, std::future<Utilities::Memory::Handle>>> futures;
			const bool loading( Utilities::GUID ID )const
			{
				for ( auto& p : futures )
					if ( p.first == ID )
						return true;
				return false;
			}
			void add_to_load( const Utilities::GUID ID )noexcept;
			std::thread thread;
			void entry( bool* running )noexcept;


			Loader_Raw( std::shared_ptr<IResourceArchive> archive,
						Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator ) : archive( archive ), allocator( allocator )
			{}
			std::shared_ptr<IResourceArchive> archive;
			Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator;
		}loader_raw;


		Utilities::CircularFiFo<Utilities::Memory::Handle> loaded_pass1;
	};
}
ENUM_FLAGS( Resources::Pass );
#endif