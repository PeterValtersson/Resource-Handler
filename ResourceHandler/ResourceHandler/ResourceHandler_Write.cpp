#include "ResourceHandler_Write.h"
#include <Utilities/Profiler/Profiler.h>
#include <windows.h>
#include <Utilities/Console/Console.h>

ResourceHandler::ResourceHandler_Write::ResourceHandler_Write(std::shared_ptr<IResourceArchive> archive)
	: archive(archive), allocator(1_gb / Utilities::Memory::ChunkyAllocator::blocksize())
{

}

ResourceHandler::ResourceHandler_Write::~ResourceHandler_Write()
{

}
void ResourceHandler::ResourceHandler_Write::set_parser_memory_handler(std::shared_ptr<Utilities::Memory::Allocator> memory_handler)
{
	parsed_memory_handler = memory_handler;
}
std::shared_ptr<ResourceHandler::IResourceArchive> ResourceHandler::ResourceHandler_Write::get_archive()
{
	return archive;
}
void ResourceHandler::ResourceHandler_Write::add_parser(const Utilities::GUID type, const std::string& library_path)
{
	if (parsers.Exists(type))
		return;

	if (!libraries.Exists(library_path))
	{
		HINSTANCE IDDLL = LoadLibrary("AssimpInterface.dll");
		if (IDDLL == 0)
			return;

		libraries.Add(library_path, IDDLL);
	}

	auto library = libraries.Get(library_path);

	Parsers::ParserData data;
	data.parse = (parse_callback_signature)GetProcAddress(library, "parse");

	if (data.parse == 0)
		return;

	parsers.Add(type, data);

}
void ResourceHandler::ResourceHandler_Write::add_parser(const Utilities::GUID type, const parse_callback& parse_callback)
{
	if (parsers.Exists(type))
		return;


	Parsers::ParserData data;
	data.parse = parse_callback;

	parsers.Add(type, data);
}
void ResourceHandler::ResourceHandler_Write::save_all()
{
	PROFILE;
	Utilities::console_print("Save all");
	To_Save_Vector to_save;
	auto& ids = resources.peek<Entries::ID>();
	auto& has_changed = resources.get<Entries::HasChanged>();
	auto& handles = resources.peek<Entries::ArchiveMemory>();
	for (size_t i = 0; i < resources.size(); i++)
	{
		if (has_changed[i])
		{
			to_save.push_back({ ids[i],  handles[i] });
			has_changed[i] = false;
		}
	}
	if (to_save.size() > 0)
		archive->save_multiple(to_save, allocator);
}
void ResourceHandler::ResourceHandler_Write::register_resource(const Utilities::GUID ID, const Flags flag)noexcept
{
	PROFILE;
	if (auto find = resources.find(ID); find.has_value())
	{
		if (flag_has(flag, Flags::Modifying))
		{
			set_flag(ID, flag);
		}
	}
	else
		resources.add(ID, Status::None, Utilities::Memory::null_handle, Utilities::Memory::null_handle, 0, false, flag);
}

ResourceHandler::Status ResourceHandler::ResourceHandler_Write::get_status(const Utilities::GUID ID) noexcept
{
	PROFILE;
	if (auto find = resources.find(ID); !find.has_value())
		return Status::NotFound;
	else
	{
		return resources.peek<Entries::Status>(*find);
	}
}

void ResourceHandler::ResourceHandler_Write::set_flag(const Utilities::GUID ID, const Flags flag) noexcept
{
	PROFILE;
	if (auto find = resources.find(ID); !find.has_value())
		return; // throw ResourceNotFound( ID );
	else
	{
		resources.get<Entries::Flags>(*find) |= flag;
	}
}

void ResourceHandler::ResourceHandler_Write::remove_flag(const Utilities::GUID ID, const Flags flag) noexcept
{
	PROFILE;
	if (auto find = resources.find(ID); !find.has_value())
		return; // throw ResourceNotFound( ID );
	else
	{
		resources.get<Entries::Flags>(*find) &= ~flag;
	}
}

void ResourceHandler::ResourceHandler_Write::inc_refCount(const Utilities::GUID ID) noexcept
{
	PROFILE;
	if (auto find = resources.find(ID); !find.has_value())
		return; // throw ResourceNotFound( ID );
	else
	{
		++resources.get<Entries::RefCount>(*find);
		if (archive->get_size(ID) == 0)
			throw NoResourceData(archive->get_name(ID), ID);
		else if (!flag_has(resources.peek<Entries::Status>(*find), Status::InMemory))
		{
			resources.get<Entries::ArchiveMemory>(*find) = archive->read(ID, allocator);
			resources.get<Entries::Status>(*find) = Status::InMemory;
			parse_resource(ID);
		}
	}
}

void ResourceHandler::ResourceHandler_Write::dec_refCount(const Utilities::GUID ID) noexcept
{
	PROFILE;
	if (auto find = resources.find(ID); !find.has_value())
		return; // throw ResourceNotFound( ID );
	else
	{
		--resources.get<Entries::RefCount>(*find);
	}
}

ResourceHandler::RefCount ResourceHandler::ResourceHandler_Write::get_refCount(const Utilities::GUID ID) const noexcept
{
	PROFILE;
	if (auto find = resources.find(ID); !find.has_value())
		return 0; // throw ResourceNotFound( ID );
	else
	{
		return resources.peek<Entries::RefCount>(*find);
	}
}

std::string ResourceHandler::ResourceHandler_Write::get_name(const Utilities::GUID ID) const
{
	PROFILE;
	return archive->get_name(ID);
}

void ResourceHandler::ResourceHandler_Write::use_data(const Utilities::GUID ID, const std::function<void(const Utilities::Memory::ConstMemoryBlock)>& callback)
{
	PROFILE;
	if (auto find = resources.find(ID); !find.has_value())
		return; // throw ResourceNotFound( ID );
	else
	{
		if (flag_has(resources.peek<Entries::Flags>(*find), Flags::Modifying))
			return;
		try
		{
			if (archive->get_size(ID) == 0)
				throw NoResourceData(archive->get_name(ID), ID);
			else if (!flag_has(resources.peek<Entries::Status>(*find), Status::InMemory))
			{
				resources.get<Entries::ArchiveMemory>(*find) = archive->read(ID, allocator);
				resources.get<Entries::Status>(*find) = Status::InMemory;
			}
			allocator.peek_data(resources.peek<Entries::ArchiveMemory>(*find), callback);
		}
		catch (...)
		{
			// Log
		}
	}
}

void ResourceHandler::ResourceHandler_Write::modify_data(const Utilities::GUID ID, const std::function<void(const Utilities::Memory::MemoryBlock)>& callback)
{
	PROFILE;
	if (auto find = resources.find(ID); !find.has_value())
		return; // throw ResourceNotFound( ID );
	else
	{
		try
		{
			if (!flag_has(resources.peek<Entries::Status>(*find), Status::InMemory))
			{
				resources.get<Entries::ArchiveMemory>(*find) = archive->read(ID, allocator);
				resources.get<Entries::Status>(*find) = Status::InMemory;
			}

			allocator.use_data(resources.peek<Entries::ArchiveMemory>(*find), callback);
			resources.get<Entries::Status>(*find) = Status::InMemory;
			resources.get<Entries::HasChanged>(*find) = true;
		}
		catch (...)
		{
			// Log
		}
	}
}

void ResourceHandler::ResourceHandler_Write::write_data(Utilities::GUID ID, const void* const data, const size_t size)
{
	PROFILE;
	if (auto find = resources.find(ID); !find.has_value())
		throw ResourceNotFound(ID);
	else
	{

		if (flag_has(resources.peek<Entries::Status>(*find), Status::InMemory))
		{
			allocator.write_data(resources.get<Entries::ArchiveMemory>(*find), data, size);
			resources.get<Entries::HasChanged>(*find) = true;
		}
		else
		{
			resources.get<Entries::ArchiveMemory>(*find) = allocator.allocate(size);
			allocator.write_data(resources.get<Entries::ArchiveMemory>(*find), data, size);
			resources.get<Entries::Status>(*find) = Status::InMemory;
			resources.get<Entries::HasChanged>(*find) = true;
		}

	}
}

void ResourceHandler::ResourceHandler_Write::set_type(Utilities::GUID ID, Utilities::GUID type)
{
	PROFILE;
	archive->set_type(ID, type);
}

void ResourceHandler::ResourceHandler_Write::set_name(Utilities::GUID ID, std::string_view name)
{
	PROFILE;
	archive->set_name(ID, name);
}

void ResourceHandler::ResourceHandler_Write::parse_resource(const Utilities::GUID ID)
{
	PROFILE;
	if (auto find = resources.find(ID); !find.has_value())
		throw ResourceNotFound(ID);
	else
	{
		auto type = archive->get_type(ID);
		if (!parsers.Exists(type))
			return;

		allocator.peek_data(resources.peek<Entries::ArchiveMemory>(*find), [&](const Utilities::Memory::ConstMemoryBlock data)
		{
			Utilities::Memory::Memstream data_stream((const uint8_t*)data.get_char(), data.used_size);
			parsers.Parse(type, parsed_memory_handler, &data_stream);
		});


	}
}
