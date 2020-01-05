#include "ResourceHandler_Write.h"
#include <Utilities/Profiler/Profiler.h>


Resources::ResourceHandler_Write::ResourceHandler_Write( std::shared_ptr<IResourceArchive> archive )
	: archive( archive ), allocator( 1_gb / Utilities::Memory::ChunkyAllocator::blocksize() )
{

}

Resources::ResourceHandler_Write::~ResourceHandler_Write()
{

}
void Resources::ResourceHandler_Write::save_all()
{
	PROFILE;
	To_Save_Vector to_save;
	auto& ids = resources.peek<Entries::ID>();
	auto& has_changed = resources.get<Entries::HasChanged>();
	auto& handles = resources.peek<Entries::Memory_Raw>();
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
void Resources::ResourceHandler_Write::register_resource( const Utilities::GUID ID )noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		resources.add( ID, Status::None, 0, 0, 0, false );
}

void Resources::ResourceHandler_Write::inc_refCount( const Utilities::GUID ID ) noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		return; // throw ResourceNotFound( ID );
	else
	{
		++resources.get<Entries::RefCount>( *find );
	}
}

void Resources::ResourceHandler_Write::dec_refCount( const Utilities::GUID ID ) noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		return; // throw ResourceNotFound( ID );
	else
	{
		--resources.get<Entries::RefCount>( *find );
	}
}

Resources::RefCount Resources::ResourceHandler_Write::get_refCount( const Utilities::GUID ID ) const noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		return 0; // throw ResourceNotFound( ID );
	else
	{
		return resources.peek<Entries::RefCount>( *find );
	}
}

std::string Resources::ResourceHandler_Write::get_name( const Utilities::GUID ID ) const
{
	PROFILE;
	return archive->get_name( ID );
}

void Resources::ResourceHandler_Write::use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback ) noexcept
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
			else if ( !flag_has( resources.peek<Entries::Status>( *find ), Status::In_Memory ) )
			{
				resources.get<Entries::Memory_Raw>( *find ) = archive->read( ID, allocator );
				resources.get<Entries::Status>( *find ) = Status::In_Memory;
			}
			allocator.peek_data( resources.peek<Entries::Memory_Raw>( *find ), callback );
		}
		catch ( ... )
		{
			// Log
		}
	}
}

void Resources::ResourceHandler_Write::modify_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::MemoryBlock )>& callback )
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		return; // throw ResourceNotFound( ID );
	else
	{
		try
		{
			if ( archive->get_size( ID ) > 0 && !flag_has( resources.peek<Entries::Status>( *find ), Status::In_Memory ) )
			{
				resources.get<Entries::Memory_Raw>( *find ) = archive->read( ID, allocator );
				resources.get<Entries::Status>( *find ) = Status::In_Memory;
			}
			allocator.use_data( resources.peek<Entries::Memory_Raw>( *find ), callback );
			resources.get<Entries::HasChanged>( *find ) = true;
		}
		catch ( ... )
		{
			// Log
		}
	}
}

void Resources::ResourceHandler_Write::write_data( Utilities::GUID ID, const void* const data, const size_t size )
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		throw ResourceNotFound( ID );
	else
	{

		if ( flag_has( resources.peek<Entries::Status>( *find ), Status::In_Memory ) )
		{
			allocator.write_data( resources.get<Entries::Memory_Raw>( *find ), data, size );
			resources.get<Entries::HasChanged>( *find ) = true;
		}
		else
		{
			resources.get<Entries::Memory_Raw>( *find ) = allocator.allocate( size );
			allocator.write_data( resources.get<Entries::Memory_Raw>( *find ), data, size );
			resources.get<Entries::Status>( *find ) = Status::In_Memory;
			resources.get<Entries::HasChanged>( *find ) = true;
		}

	}
}

void Resources::ResourceHandler_Write::set_type( Utilities::GUID ID, Utilities::GUID type )
{
	PROFILE;
	archive->set_type( ID, type );
}

void Resources::ResourceHandler_Write::set_name( Utilities::GUID ID, std::string_view name )
{
	PROFILE;
	archive->set_name( ID, name );
}
