#include "ResourceLoaderThread.h"
#include <Utilities/Profiler/Profiler.h>


template<typename T>
bool is_ready(const std::future<T>& f)
{
	return f.valid() && f.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
}



ResourceHandler::ResourceLoaderThread::ResourceLoaderThread(std::shared_ptr<IResourceArchive> archive, Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator)
	: archive(archive), allocator(allocator), running(true)
{
	to_load.load = false;
	thread = std::thread(&ResourceHandler::ResourceLoaderThread::run, this);
}

ResourceHandler::ResourceLoaderThread::~ResourceLoaderThread()
{
	running = false;
	if (thread.joinable())
		thread.join();
}

void ResourceHandler::ResourceLoaderThread::check_finished_loading(std::function<void(Utilities::GUID ID, Utilities::Memory::Handle handle, Status status)> do_if_finished) noexcept
{
	PROFILE;
	if (is_ready(to_load.future))
	{
		try
		{
			auto result = to_load.future.get();
			do_if_finished(to_load.ID, result, Status::InMemory);
		}
		catch (ResourceNotFound& e)
		{
			do_if_finished(to_load.ID, 0, Status::NotFound);
		}
		catch (Utilities::Memory::InvalidHandle& e)
		{
			do_if_finished(to_load.ID, 0, Status::CouldNotLoad);
		}
	}
}

void ResourceHandler::ResourceLoaderThread::start_to_load(std::function<Utilities::optional<Utilities::GUID>()> choose_to_load) noexcept
{
	PROFILE;
	
	if (!to_load.load)
	{
		if (auto id = choose_to_load(); id.has_value())
		{
			to_load.promise = decltype(to_load.promise)();
			to_load.future = to_load.promise.get_future();
			to_load.ID = *id;
			to_load.load = true;
		}
	}
}

void ResourceHandler::ResourceLoaderThread::run() noexcept
{
	PROFILE_N( "Loader Thread" );
	while (running)
	{
		PROFILE;
		if (to_load.load)
		{
			try
			{
				auto handle = allocator([this](Utilities::Memory::ChunkyAllocator& a)
					{
						return archive->read(to_load.ID, a);
					});
				to_load.promise.set_value(handle);
			}
			catch (...)
			{
				to_load.promise.set_exception(std::current_exception());
			}


			to_load.load = false;
		}
	}
}
