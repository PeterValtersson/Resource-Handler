#pragma once
#include <IResourceHandler.h>
#include <thread>
#include <Utilities/Memory/Sofa.h>
#include <Utilities/Memory/ChunkyAllocator.h>
#include <future>
#include <Utilities/Concurrent.h>
#include <IResourceArchive.h>
#include <thread>

namespace ResourceHandler
{
	struct Load_Parse_Info {
		Utilities::GUID ID;
		std::promise<Utilities::Memory::Handle> promise;
		std::future<Utilities::Memory::Handle> future;
		bool load;
	};

	class ResourceLoaderThread {
	public:
		ResourceLoaderThread(std::shared_ptr<IResourceArchive> archive, Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator);
		~ResourceLoaderThread();


		void check_finished_loading(
			std::function<void(Utilities::GUID ID, Utilities::Memory::Handle handle, Status status)> do_if_finished)noexcept;

		void start_to_load(std::function<Utilities::optional<Utilities::GUID>()> choose_to_load)noexcept;

	private:
		void run()noexcept;

		bool running;
		Load_Parse_Info to_load;
		std::thread thread;

		std::shared_ptr<IResourceArchive> archive;
		Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator;
	};
}