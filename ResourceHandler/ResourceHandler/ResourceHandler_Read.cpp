#include "ResourceHandler_Read.h"
#include <Utilities/Profiler/Profiler.h>

template<typename T>
bool is_ready( const std::future<T>& f )
{
	return f.wait_for( std::chrono::milliseconds( 0 ) ) == std::future_status::ready;
}



using namespace std::chrono_literals;






Resources::ResourceHandler_Read::ResourceHandler_Read( std::shared_ptr<IResourceArchive> archive )
	: archive( archive ), allocator( 1_gb / Utilities::Memory::ChunkyAllocator::blocksize() ), loader_raw(archive, allocator )
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
		process_resource_actions_queues();
		send_resouces_for_raw_loading();
		process_resouces_from_raw_loading();
		std::this_thread::sleep_for( 32ms );
	}
}

void Resources::ResourceHandler_Read::send_resouces_for_raw_loading() noexcept
{
	PROFILE;
	auto passes = resources.get<Entries::passes_loaded>();
	auto ids = resources.get<Entries::ID>();
	for ( size_t i = 0; i < resources.size(); i++ ) // Change to pick N random resources and check if they are loaded (pass0)
	{
		if ( !(passes[i] & Pass::Loaded_Raw) && archive->get_size( ids[i] ) > 0 ) // Change to only load if we have memory to spare
		{
			if ( !loader_raw.loading( ids[i] ) )
				loader_raw.add_to_load( ids[i] );
		}
	}
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
	auto passes = resources.get<Entries::passes_loaded>();
	auto raw_handle = resources.get<Entries::raw_handle>();
	for ( size_t i = 0; i < loader_raw.futures.size(); i++ )
	{
		try
		{
			if ( is_ready( loader_raw.futures[i].second ) )
			{
				if ( auto find = resources.find( loader_raw.futures[i].first ); find.has_value() )
				{
					raw_handle[*find] = loader_raw.futures[i].second.get();
					passes[*find] |= Pass::Loaded_Raw;
				}
				else
					log.push_back( "Resource has been unregistered before finishing raw_loading. GUID: " + std::to_string( loader_raw.futures[i].first ) );

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

void Resources::ResourceHandler_Read::register_resource( Utilities::GUID ID )
{
	PROFILE;
	if ( !archive->exists( ID ) )
		throw ResourceNotFound( ID );
	register_resource_queue.push( ID );
}

void Resources::ResourceHandler_Read::inc_refCount( Utilities::GUID ID )noexcept
{
	PROFILE;
	inc_refCount_queue.push( ID );
}

void Resources::ResourceHandler_Read::dec_refCount( Utilities::GUID ID )noexcept
{
	PROFILE;
	dec_refCount_queue.push( ID );
}

Resources::RefCount Resources::ResourceHandler_Read::get_refCount( Utilities::GUID ID ) const noexcept
{
	PROFILE;
	std::promise<RefCount> p;
	auto f = p.get_future();
	get_refCount_queue.push( { ID, std::move( p ) } );
	return f.get();
}

void Resources::ResourceHandler_Read::use_data( Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback )
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

void Resources::ResourceHandler_Read::process_resource_actions_queues()
{
	PROFILE;
	register_resources();
	inc_refCounts();
	dec_refCounts();
	get_refCounts();
	use_datas();
}

void Resources::ResourceHandler_Read::register_resources()
{
	PROFILE;
	while ( !register_resource_queue.isEmpty() )
	{
		const auto r = resources.add( register_resource_queue.top() );
		resources.set<Entries::passes_loaded>( r, Pass::Unloaded );
		register_resource_queue.pop();
	}
}

void Resources::ResourceHandler_Read::inc_refCounts()
{
	PROFILE;
	while ( !inc_refCount_queue.isEmpty() )
	{
		if ( const auto r = resources.find( inc_refCount_queue.top() ); !r.has_value() )
			"Write to log";
		else
			++resources.get<Entries::ref_count>( *r );
		inc_refCount_queue.pop();
	}
}

void Resources::ResourceHandler_Read::dec_refCounts()
{
	PROFILE;
	while ( !dec_refCount_queue.isEmpty() )
	{
		if ( const auto r = resources.find( dec_refCount_queue.top() ); !r.has_value() )
			"Write to log";
		else
			--resources.get<Entries::ref_count>( *r );
		dec_refCount_queue.pop();
	}
}

void Resources::ResourceHandler_Read::get_refCounts()
{
	PROFILE;
	while ( !get_refCount_queue.isEmpty() )
	{
		auto& top = get_refCount_queue.top();
		if ( const auto r = resources.find( top.ID ); !r.has_value() )
		{
			"Write to log";
			top.promise.set_value( 0 );
		}
		else
			top.promise.set_value( resources.peek<Entries::ref_count>( *r ) );

		get_refCount_queue.pop();
	}
}

void Resources::ResourceHandler_Read::use_datas()
{
	PROFILE;
	if ( !use_data_queue.isEmpty() )
	{
		auto& top = use_data_queue.top();
		try
		{
			if ( const auto r = resources.find( top.ID ); !r.has_value() )
			{
				throw ResourceNotFound( top.ID );
			}
			else if ( archive->get_size( top.ID ) == 0 )
			{
				throw NoResourceData( top.ID );
			}
			else if ( resources.peek<Entries::passes_loaded>( *r )& Pass::Loaded_Raw )
			{
				top.promise.set_value( resources.peek<Entries::raw_handle>( *r ) );
				use_data_queue.pop();
			}

		}
		catch ( ... )
		{
			top.promise.set_exception( std::current_exception() );
			use_data_queue.pop();
		}

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
