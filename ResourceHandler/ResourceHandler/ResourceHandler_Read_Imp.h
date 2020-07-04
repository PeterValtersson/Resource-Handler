#pragma once
#include <IResourceHandler.h>
#include <Utilities/GUID.h>
#include <Utilities/CircularFIFO.h>
#include <thread>
#include <Utilities/Memory/Sofa.h>
#include <Utilities/Memory/ChunkyAllocator.h>
#include <future>
#include <Utilities/Concurrent.h>
namespace ResourceHandler
{
	class ResourceHandler_Read_Imp {
	public:
		ResourceHandler_Read_Imp( std::shared_ptr<IResourceArchive> archive, Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator );
		~ResourceHandler_Read_Imp( );
		void run(); 

		void register_resource( Utilities::GUID ID )noexcept;
		Status get_status( Utilities::GUID ID )noexcept;
		void inc_refCount( Utilities::GUID ID )noexcept;
		void dec_refCount( Utilities::GUID ID )noexcept;
		RefCount get_refCount( Utilities::GUID ID )noexcept;
		Utilities::Memory::Handle get_handle( Utilities::GUID ID );

	private:
		
		struct Entries : public Utilities::Memory::SofA<Utilities::GUID, Utilities::GUID::Hasher,
			Status, // Status,
			Utilities::Memory::Handle,	 // Raw
			Utilities::Memory::Handle,	 // Parsed / VRAM info(Vertex Count, etc.)
			RefCount
		> {
			enum {
				ID,
				Status,
				Memory_Raw,
				Memory_Parsed,
				RefCount
			};
		} resources;

		std::shared_ptr<IResourceArchive> archive;
		Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator;
		std::vector<std::string> log;

		struct Load_Parse_Info {
			Utilities::GUID ID;
			std::promise<Utilities::Memory::Handle> promise;
			std::future<Utilities::Memory::Handle> future;
			bool load;
		};

		class Loader {
		public:
			Loader( std::shared_ptr<IResourceArchive> archive, Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator )
				: archive( archive ), allocator( allocator ), running( false )
			{}
			void start()noexcept;
			void stop()noexcept;
			void update(
				std::function<void( Utilities::GUID ID, Utilities::Memory::Handle handle, Status status )> do_if_finished,
				std::function<Utilities::optional<Utilities::GUID>()> choose_to_load )noexcept;

			void run()noexcept;
		private:


			bool running;
			Load_Parse_Info to_load;
			std::thread thread;

			std::shared_ptr<IResourceArchive> archive;
			Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator;
		}loader;
		void resource_loading_finished( Utilities::GUID ID, Utilities::Memory::Handle handle, Status status ); //Messes with std::function/std::bind noexcept;
		Utilities::optional<Utilities::GUID> choose_resource_to_load(); //Messes with std::function/std::bind noexcept;

	};


}