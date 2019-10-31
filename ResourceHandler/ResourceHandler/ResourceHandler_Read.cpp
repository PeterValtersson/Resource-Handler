#include "ResourceHandler_Read.h"
#include <Utilities/Profiler/Profiler.h>

template<typename T>
bool is_ready( const std::future<T>& f )
{
	return f.wait_for( std::chrono::milliseconds( 0 ) ) == std::future_status::ready;
}



using namespace std::chrono_literals;






Resources::ResourceHandler_Read::ResourceHandler_Read( std::shared_ptr<IResourceArchive> archive )
	: archive( archive ), allocator( 1_gb / Utilities::Memory::ChunkyAllocator::blocksize() ), loader_raw( archive, allocator )
{
	running = true;
	thread = std::thread( &ResourceHandler_Read::update, this );
	loader_raw.thread = std::thread( &Loader_Raw::entry, &loader_raw, &running );
}

Resources::ResourceHandler_Read::~ResourceHandler_Read()
{
	running = false;
	if ( thread.joinable() )
		thread.join();
	if ( loader_raw.thread.joinable() )
		loader_raw.thread.join();
}

void Resources::ResourceHandler_Read::update()noexcept
{
	PROFILE;
	while ( running )
	{
		send_resouces_for_raw_loading();
		process_resouces_from_raw_loading();
		use_datas();
		std::this_thread::sleep_for( 32ms );
	}
}

void Resources::ResourceHandler_Read::send_resouces_for_raw_loading() noexcept
{
	PROFILE;
	resources( [this]( Entries& r )
	{
		auto passes = r.get<Entries::passes_loaded>();
		auto ids = r.get<Entries::ID>();
		for ( size_t i = 0; i < r.size(); i++ ) // Change to pick N random resources and check if they are loaded (pass0)
		{
			if ( !(passes[i] & Pass::Loaded_Raw) && archive->get_size( ids[i] ) > 0 ) // Change to only load if we have memory to spare
			{
				if ( !loader_raw.loading( ids[i] ) )
					loader_raw.add_to_load( ids[i] );
			}
		}

	} );

}

template<class T>
void remove_element_replace_with_last( std::vector<T>& t, size_t i )
{
	t[i] = std::move( t.back() );
	t.pop_back();
}

void Resources::ResourceHandler_Read::process_resouces_from_raw_loading() noexcept
{
	PROFILE;
	for ( size_t i = 0; i < loader_raw.futures.size(); i++ )
	{
		try
		{
			if ( is_ready( loader_raw.futures[i].second ) )
			{
				resources( [this, i]( Entries& r )
				{
					if ( auto find = r.find( loader_raw.futures[i].first ); find.has_value() )
					{
						r.get<Entries::raw_handle>( *find ) = loader_raw.futures[i].second.get();
						r.get<Entries::passes_loaded>( *find ) |= Pass::Loaded_Raw;
					}
					else
						log.push_back( "Resource has been unregistered before finishing raw_loading. GUID: " + std::to_string( loader_raw.futures[i].first ) );
				} );


				remove_element_replace_with_last( loader_raw.futures, i );
				--i;
			}
		}
		catch ( std::exception & e )
		{
			log.push_back( e.what() + std::string( " GUID: " ) + std::to_string( loader_raw.futures[i].first ) );
			remove_element_replace_with_last( loader_raw.futures, i );
			--i;
		}
	}

}

void Resources::ResourceHandler_Read::register_resource( const Utilities::GUID ID )
{
	PROFILE;
	if ( !archive->exists( ID ) )
		throw ResourceNotFound( ID );
	resources( [ID]( Entries& r )
	{
		const auto index = r.add( ID );
		r.set<Entries::passes_loaded>( index, Pass::Unloaded );
	} );
}

void Resources::ResourceHandler_Read::inc_refCount( const Utilities::GUID ID )noexcept
{
	PROFILE;
	resources( [ID]( Entries& r )
	{
		if ( const auto i = r.find( ID ); !i.has_value() )
			"Write to log";
		else
			++r.get<Entries::ref_count>( *i );
	} );
}

void Resources::ResourceHandler_Read::dec_refCount( const Utilities::GUID ID )noexcept
{
	PROFILE;
	resources( [ID]( Entries& r )
	{
		if ( const auto i = r.find( ID ); !i.has_value() )
			"Write to log";
		else
			--r.get<Entries::ref_count>( *i );
	} );
}

Resources::RefCount Resources::ResourceHandler_Read::get_refCount( const Utilities::GUID ID ) const noexcept
{
	PROFILE;
	return resources( [ID]( Entries& r )-> RefCount
	{
		if ( const auto i = r.find( ID ); !i.has_value() )
		{
			"Write to log";
			return 0;
		}
		else
			return r.peek<Entries::ref_count>( *i );
	} );
}

void Resources::ResourceHandler_Read::use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback )const
{
	PROFILE;
	std::promise<Utilities::Memory::Handle> p;
	auto f = p.get_future();
	use_data_queue.push( { ID, std::move( p ) } );

	const auto handle = f.get();
	allocator( [&]( Utilities::Memory::ChunkyAllocator& a )
	{
		a.peek_data( handle, callback );
	} );

}

void Resources::ResourceHandler_Read::use_datas()const noexcept
{
	PROFILE;
	if ( !use_data_queue.isEmpty() )
	{
		auto& top = use_data_queue.top();
		try
		{
			resources( [&top, this]( const Entries& r )
			{
				if ( const auto i = r.find( top.ID ); !i.has_value() )
					throw ResourceNotFound( top.ID );
				else if ( archive->get_size( top.ID ) == 0 )
					throw NoResourceData( top.ID );
				else if ( r.peek<Entries::passes_loaded>( *i )& Pass::Loaded_Raw )
					top.promise.set_value( r.peek<Entries::raw_handle>( *i ) );
				else
					use_data_queue.push( std::move( top ) );
			} );
		}
		catch ( ... )
		{
			top.promise.set_exception( std::current_exception() );
		}
		use_data_queue.pop();
	}

}

void Resources::ResourceHandler_Read::Loader_Raw::add_to_load( const Utilities::GUID ID )noexcept
{
	PROFILE;
	std::promise<Utilities::Memory::Handle> p;
	futures.push_back( { ID, std::move( p.get_future() ) } );
	to_load.push( { ID, std::move( p ) } );
}

void Resources::ResourceHandler_Read::Loader_Raw::entry( bool* running ) noexcept
{
	PROFILE;
	while ( *running )
	{
		while ( !to_load.isEmpty() )
		{
			PROFILE_N( Load_Raw );
			auto& top = to_load.top();
			if ( archive->exists( top.first ) )
			{
				allocator( [&]( Utilities::Memory::ChunkyAllocator& a )
				{
					try
					{
						auto handle = archive->read( top.first, a );
						top.second.set_value( handle );
					}
					catch ( ... )
					{
						top.second.set_exception( std::current_exception() );
					}
				} );
			}

			to_load.pop();
		}
		std::this_thread::sleep_for( 32ms );
	}
}
