#include "ResourceHandler_Write.h"
#include <Utilities/Profiler/Profiler.h>
#include <windows.h>

ResourceHandler::ResourceHandler_Write::ResourceHandler_Write( std::shared_ptr<IResourceArchive> archive )
	: archive( archive ), allocator( 1_gb / Utilities::Memory::ChunkyAllocator::blocksize() )
{

}

ResourceHandler::ResourceHandler_Write::~ResourceHandler_Write()
{

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
void ResourceHandler::ResourceHandler_Write::save_all()
{
	PROFILE;
	To_Save_Vector to_save;
	auto& ids = resources.peek<Entries::ID>();
	auto& has_changed = resources.get<Entries::HasChanged>();
	auto& handles = resources.peek<Entries::Memory>();
	for ( size_t i = 0; i < resources.size(); i++ )
	{
		if ( has_changed[i] )
		{
			to_save.push_back( { ids[i],  handles[i] } );
			has_changed[i] = false;
		}
	}
	if ( to_save.size() > 0 )
		archive->save_multiple( to_save, allocator );
}
void ResourceHandler::ResourceHandler_Write::register_resource( const Utilities::GUID ID )noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		resources.add( ID, Status::None, 0, 0, false );
}

ResourceHandler::Status ResourceHandler::ResourceHandler_Write::get_status( const Utilities::GUID ID ) noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		return Status::NotFound;
	else
	{
		return resources.peek<Entries::Status>( *find );
	}
}

void ResourceHandler::ResourceHandler_Write::inc_refCount( const Utilities::GUID ID ) noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		return; // throw ResourceNotFound( ID );
	else
	{
		++resources.get<Entries::RefCount>( *find );
	}
}

void ResourceHandler::ResourceHandler_Write::dec_refCount( const Utilities::GUID ID ) noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		return; // throw ResourceNotFound( ID );
	else
	{
		--resources.get<Entries::RefCount>( *find );
	}
}

ResourceHandler::RefCount ResourceHandler::ResourceHandler_Write::get_refCount( const Utilities::GUID ID ) const noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		return 0; // throw ResourceNotFound( ID );
	else
	{
		return resources.peek<Entries::RefCount>( *find );
	}
}

std::string ResourceHandler::ResourceHandler_Write::get_name( const Utilities::GUID ID ) const
{
	PROFILE;
	return archive->get_name( ID );
}

void ResourceHandler::ResourceHandler_Write::use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback ) 
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		return; // throw ResourceNotFound( ID );
	else
	{
		try
		{
			if ( archive->get_size( ID ) == 0 )
				throw NoResourceData( archive->get_name( ID ), ID );
			else if ( !flag_has( resources.peek<Entries::Status>( *find ), Status::InMemory ) )
			{
				resources.get<Entries::Memory>( *find ) = archive->read( ID, allocator );
				resources.get<Entries::Status>( *find ) = Status::InMemory;
			}
			allocator.peek_data( resources.peek<Entries::Memory>( *find ), callback );
		}
		catch ( ... )
		{
			// Log
		}
	}
}

void ResourceHandler::ResourceHandler_Write::modify_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::MemoryBlock )>& callback )
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		return; // throw ResourceNotFound( ID );
	else
	{
		try
		{
			if ( archive->get_size( ID ) > 0 && !flag_has( resources.peek<Entries::Status>( *find ), Status::InMemory ) )
			{
				resources.get<Entries::Memory>( *find ) = archive->read( ID, allocator );
				resources.get<Entries::Status>( *find ) = Status::InMemory;
			}
			allocator.use_data( resources.peek<Entries::Memory>( *find ), callback );
			resources.get<Entries::HasChanged>( *find ) = true;
		}
		catch ( ... )
		{
			// Log
		}
	}
}

void ResourceHandler::ResourceHandler_Write::write_data( Utilities::GUID ID, const void* const data, const size_t size )
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		throw ResourceNotFound( ID );
	else
	{

		if ( flag_has( resources.peek<Entries::Status>( *find ), Status::InMemory ) )
		{
			allocator.write_data( resources.get<Entries::Memory>( *find ), data, size );
			resources.get<Entries::HasChanged>( *find ) = true;
		}
		else
		{
			resources.get<Entries::Memory>( *find ) = allocator.allocate( size );
			allocator.write_data( resources.get<Entries::Memory>( *find ), data, size );
			resources.get<Entries::Status>( *find ) = Status::InMemory;
			resources.get<Entries::HasChanged>( *find ) = true;
		}

	}
}

void ResourceHandler::ResourceHandler_Write::set_type( Utilities::GUID ID, Utilities::GUID type )
{
	PROFILE;
	archive->set_type( ID, type );
}

void ResourceHandler::ResourceHandler_Write::set_name( Utilities::GUID ID, std::string_view name )
{
	PROFILE;
	archive->set_name( ID, name );
}
