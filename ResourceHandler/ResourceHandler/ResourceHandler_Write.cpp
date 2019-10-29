#include "ResourceHandler_Write.h"
#include <Utilities/Profiler/Profiler.h>

void Resources::ResourceHandler_Write::register_resource( Utilities::GUID ID )
{
	PROFILE;
	if (!archive->exists( ID ))
		archive->create( ID );

	ResourceHandler_Read::register_resource( ID );
}

void Resources::ResourceHandler_Write::write_data( Utilities::GUID ID, const char* const data, size_t size )
{
	std::promise<Utilities::Memory::Handle> p;
	auto f = p.get_future();
	write_data_queue.push( { ID, std::move( p ) } );

	const auto handle = f.get();
	allocator( [&]( Utilities::Memory::ChunkyAllocator& a )
	{
		if ( handle == Utilities::Memory::null_handle )
		{
			a.allocate( size );
		}
	} );
	
	
}

void Resources::ResourceHandler_Write::set_type( Utilities::GUID ID )
{
}

void Resources::ResourceHandler_Write::set_name( Utilities::GUID ID, std::string_view name )
{
}

void Resources::ResourceHandler_Write::write_datas()
{
	while ( !write_data_queue.isEmpty() )
	{
		auto& top = write_data_queue.top();
		try
		{
			if ( const auto find = resources.find( top.ID ); !find.has_value() )
				throw ResourceNotFound( top.ID );
			else if ( resources.peek<Entries::passes_loaded>( *find )& Pass::Loaded_Raw )
				top.promise.set_value( resources.peek<Entries::raw_handle>( *find ) );
			else
				top.promise.set_value( Utilities::Memory::null_handle );
		}
		catch ( ... )
		{
			top.promise.set_exception( std::current_exception() );
		}
		write_data_queue.pop();
	}

}
