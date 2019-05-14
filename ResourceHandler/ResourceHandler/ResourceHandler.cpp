#include "ResourceHandler.h"
#include <Profiler.h>

template<typename T>
bool is_ready( const std::future<T>& f )
{
	return f.wait_for( std::chrono::milliseconds( 0 ) ) == std::future_status::ready;
}










Resources::ResourceHandler::ResourceHandler( std::vector<std::unique_ptr<IResourceArchive>>& archives )
	: archives( std::move( archives ) ), allocator( uint32_t( 1_gb / Utilities::Allocators::ChunkyAllocator::blocksize() ) )
{
	running = true;
	pass0.thread = std::thread( &Pass0::entry, &pass0, &running, this );
}

Resources::ResourceHandler::~ResourceHandler()
{
	running = false;
	pass0.thread.join();
}

void Resources::ResourceHandler::update()noexcept
{
	Profile;

	auto refCounts = resources.get<Entries::RefCount>();
	auto states = resources.get<Entries::State>();
	auto ids = resources.get<Entries::ID>();
	auto handles = resources.get<Entries::Handle>();
	for ( size_t i = 0; i < resources.size(); i++ )
	{
		if ( refCounts[i] > 0 && !(states[i] & Resource_State::Pass0) )
		{
			if ( !pass0.loading( ids[i] ) )
			{
				std::promise<Utilities::Allocators::Handle> p;
				pass0.futures.push_back( { ids[i], std::move( p.get_future() ) } );
				pass0.to_load.push( { ids[i], std::move( p ) } );
			}
		}
	}

	for ( size_t i = 0; i < pass0.futures.size(); i++ )
	{
		try
		{
			if ( is_ready( pass0.futures[i].second ) )
			{
				if ( auto find = resources.find( pass0.futures[i].first ); find.has_value() )
				{
					handles[*find] = pass0.futures[i].second.get();
					states[*find] |= Resource_State::Pass0;
				}
			}
		} catch ( std::exception& e )
		{
			log.push_back( e.what() + std::string( " GUID: " ) + std::to_string( pass0.futures[i].first ) );
		}
	}
}

void Resources::ResourceHandler::register_resource( Utilities::GUID ID )noexcept
{
	Profile;
	resources.add( ID );
}

void Resources::ResourceHandler::inc_refCount( Utilities::GUID ID )
{
	Profile;
	if ( auto find = resources.find( ID ); !find.has_value() )
		"Write to log";
	else
		resources.get<Entries::RefCount>( *find )++;
}

void Resources::ResourceHandler::dec_refCount( Utilities::GUID ID )
{
	Profile;
	if ( auto find = resources.find( ID ); !find.has_value() )
		"Write to log";
	else
		resources.get<Entries::RefCount>( *find )--;
}

Resources::RefCount Resources::ResourceHandler::get_refCount( Utilities::GUID ID ) const
{
	Profile;
	if ( auto find = resources.find( ID ); !find.has_value() )
		"Write to log";
	else
		return resources.peek<Entries::RefCount>( *find );
	return 0;
}

void Resources::ResourceHandler::use_data( Utilities::GUID ID, const std::function<void( const Utilities::Allocators::MemoryBlock )>& callback )
{
	Profile;
	if ( auto find = resources.find( ID ); !find.has_value() )
		"Write to log";
	else
	{
		while ( !(resources.peek<Entries::State>( *find ) & Resource_State::Pass0) )
			update();

		allocator.use_data( resources.peek<Entries::Handle>( *find ), callback );
	}
}

void Resources::ResourceHandler::Pass0::entry( bool* running, ResourceHandler* rh )
{
	while ( *running )
		if ( !to_load.isEmpty() )
		{
			auto& top = to_load.top();
			for ( auto& archive : rh->archives )
			{
				if ( archive->exists( top.first ) )
				{
					try
					{
						auto handle = archive->read( top.first, rh->allocator );
						top.second.set_value( handle );
						break;
					} catch ( ... )
					{
						top.second.set_exception( std::current_exception() );
					}

				}
			}
			to_load.pop();
		}
}
