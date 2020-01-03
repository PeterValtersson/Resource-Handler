#ifndef _RESOURCE_HANDLER_H_
#define _RESOURCE_HANDLER_H_
#include <IResourceHandler.h>
#include <Utilities/CircularFIFO.h>
#include <thread>
#include <Utilities/Memory/Sofa.h>
#include <Utilities/Memory/ChunkyAllocator.h>
#include <future>
#include <Utilities/Concurrent.h>

namespace Resources
{
	class ResourceHandler_Read : public IResourceHandler {
	public:
		ResourceHandler_Read(std::shared_ptr<IResourceArchive> archive);
		~ResourceHandler_Read();


	protected:
		virtual void			register_resource(const Utilities::GUID ID)noexcept override;
		virtual void			inc_refCount(const Utilities::GUID ID)noexcept  override;
		virtual void			dec_refCount(const Utilities::GUID ID)noexcept  override;
		virtual RefCount		get_refCount(const Utilities::GUID ID)const noexcept override;
		virtual void			use_data(const Utilities::GUID ID, const std::function<void(const Utilities::Memory::ConstMemoryBlock)>& callback)const override;



		Utilities::CircularFiFo<Utilities::GUID> to_register;
		void register_resources()noexcept;
		
		Utilities::CircularFiFo<Utilities::GUID> to_inc_refCount;
		void inc_refCounts()noexcept;

		Utilities::CircularFiFo<Utilities::GUID> to_dec_refCount;
		void dec_refCounts()noexcept;

		struct get_refCounts_info {
			Utilities::GUID ID;
			std::promise<RefCount> promise;
		};
		mutable Utilities::CircularFiFo<get_refCounts_info> to_get_refCount;
		void get_refCounts()const noexcept;

		struct use_data_info {
			Utilities::GUID ID;
			std::promise<Utilities::Memory::Handle> promise;
		};
		mutable Utilities::CircularFiFo_Multiple_Producers<use_data_info>	use_data_queue;
		void																use_datas()const noexcept;



		struct Entries : public Utilities::Memory::SofA<Utilities::GUID, Utilities::GUID::Hasher,
			Status, // Status,
			Utilities::Memory::Handle,	 // Raw
			Utilities::Memory::Handle,	 // Parsed / VRAM info(Vertex Count, etc.)
			RefCount
		> {
			static const uint8_t ID = 0;
			static const uint8_t Status = 1;
			static const uint8_t Memory_Raw = 2;
			static const uint8_t Memory_Parsed = 3;
			static const uint8_t RefCount = 4;
		} resources;

		void update()noexcept;
	
		std::shared_ptr<IResourceArchive> archive;
		Utilities::Concurrent<Utilities::Memory::ChunkyAllocator> allocator;
		std::thread thread;
		std::vector<std::string> log;
		bool running;

		struct Load_Parse_Info {
			struct Result {
				Status status;
				Utilities::Memory::Handle handle;
			};
			Utilities::GUID ID;
			std::promise<Result> promise;
			std::future<Result> future;
			bool load;
		};

		class Loader
		{
		public:
			Loader(std::shared_ptr<IResourceArchive> archive, Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator) 
				: archive(archive), allocator(allocator)
			{}
			void start()noexcept;
			void stop()noexcept;
			void update(
				std::function<void(Utilities::GUID ID, Utilities::Memory::Handle handle, Status status)> do_if_finished,
				std::function<Utilities::optional<Utilities::GUID>()> choose_to_load)noexcept;
		private:
			void run()noexcept;

			bool running;	
			Load_Parse_Info to_load;
			std::thread thread;

			std::shared_ptr<IResourceArchive> archive;
			Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator;
		}loader;
		void resource_loading_finished(Utilities::GUID ID, Utilities::Memory::Handle handle, Status status); //Messes with std::function/std::bind noexcept;
		Utilities::optional<Utilities::GUID> choose_resource_to_load(); //Messes with std::function/std::bind noexcept;
	};
}
#endif