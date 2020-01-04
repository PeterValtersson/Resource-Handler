#include "ResourceHandler_Read.h"
#include <Utilities/Profiler/Profiler.h>

template<typename T>
bool is_ready(const std::future<T>& f)
{
	return f.valid() && f.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
}



using namespace std::chrono_literals;






Resources::ResourceHandler_Read::ResourceHandler_Read(std::shared_ptr<IResourceArchive> archive)
	: archive(archive), allocator(1_gb / Utilities::Memory::ChunkyAllocator::blocksize()), loader(archive, allocator)
{
	running = true;
	thread = std::thread(&ResourceHandler_Read::update, this);
	loader.start();
}

Resources::ResourceHandler_Read::~ResourceHandler_Read()
{
	running = false;
	if (thread.joinable())
		thread.join();
	loader.stop();
}

void Resources::ResourceHandler_Read::update()noexcept
{
	using namespace std::placeholders;
	PROFILE("Resource Handler Thread");
	while (running)
	{
		PROFILE("Running");
		register_resources();
		inc_refCounts();
		dec_refCounts();
		get_refCounts();

		loader.update(std::bind(&ResourceHandler_Read::resource_loading_finished, this, _1, _2, _3),
			std::bind(&ResourceHandler_Read::choose_resource_to_load, this));

		use_datas();
		loader.run();
		std::this_thread::sleep_for(32ms);
	}
}
void Resources::ResourceHandler_Read::resource_loading_finished(Utilities::GUID ID, Utilities::Memory::Handle handle, Status status)
{
	PROFILE;
	if (auto find = resources.find(ID); !find.has_value())
		return; // Logg Resource no longer in use and has been unregistered.
	else
	{
		resources.get<Entries::Memory_Raw>(*find) = handle;
		resources.get<Entries::Status>(*find) |= status;
		resources.get<Entries::Status>(*find) &= ~Status::Loading;
	}
}
Utilities::optional<Utilities::GUID> Resources::ResourceHandler_Read::choose_resource_to_load()
{
	PROFILE;
	for (size_t i = 0; i < resources.size(); ++i)// TODO: Change to a suitable method for choosing resource to load  (FIFO, FILO, Highest refcount, etc.)
	{
		if (!flag_has(resources.peek<Entries::Status>(i), Status::In_Memory | Status::Could_Not_Load))
		{
			if (archive->get_size(resources.peek<Entries::ID>(i)) > 0)
			{
				resources.get<Entries::Status>(i) |= Status::Loading;
				return resources.peek<Entries::ID>(i);
			}
		}
	}
	return std::nullopt;
}

void Resources::ResourceHandler_Read::register_resource(const Utilities::GUID ID)noexcept
{
	PROFILE;
	if (!archive->exists(ID))
		return;

	to_register.push(ID);
}

void Resources::ResourceHandler_Read::inc_refCount(const Utilities::GUID ID)noexcept
{
	PROFILE;
	to_inc_refCount.push(ID);
}

void Resources::ResourceHandler_Read::dec_refCount(const Utilities::GUID ID)noexcept
{
	PROFILE;
	to_dec_refCount.push(ID);
}

Resources::RefCount Resources::ResourceHandler_Read::get_refCount(const Utilities::GUID ID) const noexcept
{
	PROFILE;
	std::promise<RefCount> p;
	auto f = p.get_future();
	to_get_refCount.push({ ID, std::move(p) });
	return p.get_future().get();
}

void Resources::ResourceHandler_Read::use_data(const Utilities::GUID ID, const std::function<void(const Utilities::Memory::ConstMemoryBlock)>& callback)const
{
	PROFILE;
	std::promise<Utilities::Memory::Handle> p;
	auto f = p.get_future();
	use_data_queue.push({ ID, std::move(p) });
	try
	{
		const auto handle = f.get();
		allocator([&](const Utilities::Memory::ChunkyAllocator& a)
			{
				a.peek_data(handle, callback);
			});
	}
	catch (...)
	{

	}
}

void Resources::ResourceHandler_Read::register_resources() noexcept
{
	PROFILE;
	while (!to_register.isEmpty())
	{
		auto& top = to_register.top();
		if (const auto find = resources.find(top); !find.has_value())
			resources.add(top, Status::None, 0, 0, 0);
		to_register.pop(); // Should always pop
	}
}

void Resources::ResourceHandler_Read::inc_refCounts() noexcept
{
	PROFILE;
	while (!to_inc_refCount.isEmpty())
	{
		auto& top = to_inc_refCount.top();
		if (const auto find = resources.find(top); !find.has_value())
		{
			++resources.get<Entries::RefCount>(*find);
			to_inc_refCount.pop(); // Only pop if resource was found (otherwise wait for the resource to be registered)
		}
		else
			break;
	}
}

void Resources::ResourceHandler_Read::dec_refCounts() noexcept
{
	PROFILE;
	while (!to_dec_refCount.isEmpty())
	{
		auto& top = to_dec_refCount.top();
		if (const auto find = resources.find(top); !find.has_value())
		{
			--resources.get<Entries::RefCount>(*find);
			to_dec_refCount.pop();
		}
		else
			break;
	}
}

void Resources::ResourceHandler_Read::get_refCounts() const noexcept
{
	PROFILE;
	while (!to_get_refCount.isEmpty())
	{
		auto& top = to_get_refCount.top();
		if (const auto find = resources.find(top.ID); !find.has_value())
		{
			top.promise.set_value(resources.peek<Entries::RefCount>(*find));
			to_get_refCount.pop();
		}
		else
			break;
	}
}

void Resources::ResourceHandler_Read::use_datas()const noexcept
{
	PROFILE;
	while (!use_data_queue.isEmpty())
	{
		auto& top = use_data_queue.top();
		if (const auto find = resources.find(top.ID); find.has_value())
		{
			try
			{
				if (archive->get_size(top.ID) == 0)
					throw NoResourceData(archive->get_name(top.ID), top.ID);
				else if (flag_has(resources.peek<Entries::Status>(*find), Status::In_Memory_Parsed))
				{
					top.promise.set_value(resources.peek<Entries::Memory_Parsed>(*find));
					use_data_queue.pop();
				}

				else if (flag_has(resources.peek<Entries::Status>(*find), Status::In_Memory))
				{
					top.promise.set_value(resources.peek<Entries::Memory_Raw>(*find));
					use_data_queue.pop();
				}
				else
					break;
			}
			catch (...)
			{
				top.promise.set_exception(std::current_exception());
				use_data_queue.pop();
			}

		}
		else
			break;
	}

}

void Resources::ResourceHandler_Read::Loader::start()noexcept
{
	to_load.load = false;
	running = true;
	//thread = std::thread(&Resources::ResourceHandler_Read::Loader::run, this);
}

void Resources::ResourceHandler_Read::Loader::stop()noexcept
{
	running = false;
	if (thread.joinable())
		thread.join();
}

void Resources::ResourceHandler_Read::Loader::update(
	std::function<void(Utilities::GUID ID, Utilities::Memory::Handle handle, Status status)> do_if_finished,
	std::function<Utilities::optional<Utilities::GUID>()> choose_to_load)noexcept
{
	if (is_ready(to_load.future))
	{
		try
		{
			auto result = to_load.future.get();
			do_if_finished(to_load.ID, result, Status::In_Memory);
		}
		catch (ResourceNotFound & e)
		{
			do_if_finished(to_load.ID, 0, Status::Not_Found);
		}
		catch (Utilities::Memory::InvalidHandle & e)
		{
			do_if_finished(to_load.ID, 0, Status::Could_Not_Load);
		}
	}

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

void Resources::ResourceHandler_Read::Loader::run()noexcept
{
	PROFILE_N("Loader Thread");
	//while (running)
	{
		PROFILE_N("Running");
		if (to_load.load)
		{
			try
			{
				auto handle = allocator([this](Utilities::Memory::ChunkyAllocator& a)
					{
						return archive->read(to_load.ID, a);
					});
				to_load.promise.set_value( handle );
			}
			catch (...)
			{
				to_load.promise.set_exception(std::current_exception());
			}


			to_load.load = false;
		}
	}
}
