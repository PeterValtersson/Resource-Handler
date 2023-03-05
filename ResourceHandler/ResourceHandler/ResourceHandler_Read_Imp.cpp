#include "ResourceHandler_Read_Imp.h"
#include <Utilities/Profiler/Profiler.h>

using namespace std::placeholders;

ResourceHandler::ResourceHandler_Read_Imp::ResourceHandler_Read_Imp(std::shared_ptr<IResourceArchive> archive, Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator) :
	archive(archive),
	allocator(allocator),
	loader_thread(archive, allocator)
{

}

ResourceHandler::ResourceHandler_Read_Imp::~ResourceHandler_Read_Imp()
{

}

void ResourceHandler::ResourceHandler_Read_Imp::run()
{
	loader_thread.check_finished_loading(std::bind(&ResourceHandler_Read_Imp::resource_loading_finished, this, _1, _2, _3));
	loader_thread.start_to_load(std::bind(&ResourceHandler_Read_Imp::choose_resource_to_load, this));
}

void ResourceHandler::ResourceHandler_Read_Imp::register_resource(Utilities::GUID ID)noexcept
{
	PROFILE;
	if (const auto find = resources.find(ID); !find.has_value())
		resources.add(ID, Status::None, 0, 0, 0);
}
ResourceHandler::Status ResourceHandler::ResourceHandler_Read_Imp::get_status(Utilities::GUID ID)noexcept
{
	if (const auto find = resources.find(ID); find.has_value())
		return resources.peek<Entries::Status>(*find);
	return Status::Not_Found;
}
void ResourceHandler::ResourceHandler_Read_Imp::inc_refCount(Utilities::GUID ID)noexcept
{
	PROFILE;
	if (const auto find = resources.find(ID); find.has_value())
		++resources.get<Entries::RefCount>(*find);
}
void ResourceHandler::ResourceHandler_Read_Imp::dec_refCount(Utilities::GUID ID)noexcept
{
	PROFILE;
	if (const auto find = resources.find(ID); find.has_value())
		--resources.get<Entries::RefCount>(*find);
}
ResourceHandler::RefCount ResourceHandler::ResourceHandler_Read_Imp::get_refCount(Utilities::GUID ID)noexcept
{
	PROFILE;
	if (const auto find = resources.find(ID); find.has_value())
		return resources.peek<Entries::RefCount>(*find);
	return RefCount(0);
}
Utilities::Memory::Handle ResourceHandler::ResourceHandler_Read_Imp::get_handle(Utilities::GUID ID)
{
	PROFILE;
	if (const auto find = resources.find(ID); !find.has_value())
		throw ResourceNotFound(ID);
	else
	{
		while (true)
		{
			if (archive->get_size(ID) == 0)
				throw NoResourceData(archive->get_name(ID), ID);
			else if (flag_has(resources.peek<Entries::Status>(*find), Status::In_Memory))
				return resources.peek<Entries::Memory_Raw>(*find);
			else if (flag_has(resources.peek<Entries::Status>(*find), Status::Could_Not_Load))
			{
				throw Utilities::Memory::InvalidHandle("");
			}
			else if (flag_has(resources.peek<Entries::Status>(*find), Status::Not_Found))
			{
				throw ResourceNotFound(ID);
			}
			else if (flag_has(resources.peek<Entries::Status>(*find), Status::Loading))
			{
				run();
			}
			else
			{
				loader_thread.check_finished_loading(std::bind(&ResourceHandler_Read_Imp::resource_loading_finished, this, _1, _2, _3));
				loader_thread.start_to_load([&]()
					{
						return ID;
					});
			}
		}
	}
}


void ResourceHandler::ResourceHandler_Read_Imp::resource_loading_finished(Utilities::GUID ID, Utilities::Memory::Handle handle, Status status)
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
Utilities::optional<Utilities::GUID> ResourceHandler::ResourceHandler_Read_Imp::choose_resource_to_load()
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

