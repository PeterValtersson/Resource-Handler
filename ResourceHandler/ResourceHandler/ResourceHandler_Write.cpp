//#include "ResourceHandler_Write.h"
//#include <Utilities/Profiler/Profiler.h>
//
//void Resources::ResourceHandler_Write::register_resource( Utilities::GUID ID )
//{
//	PROFILE;
//	if ( !archive->exists( ID ) )
//		archive->create( ID );
//
//	ResourceHandler_Read::register_resource( ID );
//}
//
//void Resources::ResourceHandler_Write::write_data( Utilities::GUID ID, const void* const data, const size_t size )
//{
//	auto h = resources( [ID, data, size, this]( Entries& r ) -> Utilities::Memory::Handle
//	{
//		if ( const auto i = r.find( ID ); !i.has_value() )
//			throw ResourceNotFound( ID );
//		else
//		{
//			if ( r.peek<Entries::passes_loaded>( *i )& Pass::Loaded_Raw )
//				return r.peek<Entries::raw_handle>( *i );
//			{
//				allocator( [&r, &i, data, size]( Utilities::Memory::ChunkyAllocator& a )
//				{
//					a.write_data( r.peek<Entries::raw_handle>( *i ), data, size );
//				} );
//			}
//			{
//
//			}
//		}
//	} );
//
//	std::promise<Utilities::Memory::Handle> p;
//	auto f = p.get_future();
//	write_data_queue.push( { ID, std::move( p ) } );
//
//	auto handle = f.get();
//	allocator( [&]( Utilities::Memory::ChunkyAllocator& a )
//	{
//		if ( handle == Utilities::Memory::null_handle )
//		{
//			handle = a.allocate( size );
//		}
//		a.write_data( handle, data, size );
//	} );
//
//
//}
//
//void Resources::ResourceHandler_Write::set_type( Utilities::GUID ID, Utilities::GUID type )
//{}
//
//void Resources::ResourceHandler_Write::set_name( Utilities::GUID ID, std::string_view name )
//{}
//
//void Resources::ResourceHandler_Write::write_datas()noexcept
//{
//	if ( !write_data_queue.isEmpty() )
//	{
//		auto& top = write_data_queue.top();
//		try
//		{
//			if ( const auto find = resources.find( top.ID ); !find.has_value() )
//				throw ResourceNotFound( top.ID );
//			else if ( resources.peek<Entries::passes_loaded>( *find )& Pass::Loaded_Raw )
//				top.promise.set_value( resources.peek<Entries::raw_handle>( *find ) );
//			else if ( !loader_raw.loading( top.ID ) )
//				top.promise.set_value( Utilities::Memory::null_handle );
//		}
//		catch ( ... )
//		{
//			top.promise.set_exception( std::current_exception() );
//		}
//		write_data_queue.pop();
//	}
//
//}
//
//void Resources::ResourceHandler_Write::set_types() noexcept
//{
//	while ( !write_data_queue.isEmpty() )
//	{
//		auto& top = write_data_queue.top();
//		try
//		{
//			if ( const auto find = resources.find( top.ID ); !find.has_value() )
//				throw ResourceNotFound( top.ID );
//			else if ( resources.peek<Entries::passes_loaded>( *find )& Pass::Loaded_Raw )
//				top.promise.set_value( resources.peek<Entries::raw_handle>( *find ) );
//			else
//				top.promise.set_value( Utilities::Memory::null_handle );
//		}
//		catch ( ... )
//		{
//			top.promise.set_exception( std::current_exception() );
//		}
//		write_data_queue.pop();
//	}
//}
