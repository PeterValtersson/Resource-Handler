#include "ResourceHandler_Read.h"
#include <Utilities/Profiler/Profiler.h>

template<typename T>
bool is_ready( const std::future<T>& f )
{
	return f.wait_for( std::chrono::milliseconds( 0 ) ) == std::future_status::ready;
}










Resources::ResourceHandler_Read::ResourceHandler_Read( std::shared_ptr<IResourceArchive> archive )
	: archive( archive ), allocator( uint32_t( 1_gb / Utilities::Memory::ChunkyAllocator::blocksize() ) ), pass0( archive, allocator )
{
	running = true;
	thread = std::thread( &ResourceHandler_Read::update, this );
	pass0.thread = std::thread( &Pass0::entry, &pass0, &running );
}

Resources::ResourceHandler_Read::~ResourceHandler_Read()
{
	running = false;
	thread.join();
	pass0.thread.join();
}

void Resources::ResourceHandler_Read::update()noexcept
{
	PROFILE;
	while ( running )
	{
		PROFILE_N( Update );
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
					std::promise<Utilities::Memory::Handle> p;
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
			}
			catch ( std::exception & e )
			{
				log.push_back( e.what() + std::string( " GUID: " ) + std::to_string( pass0.futures[i].first ) );
			}
		}
	}
}

void Resources::ResourceHandler_Read::register_resource( Utilities::GUID ID )
{
	PROFILE;
	if ( !archive->exists( ID ) )
		throw ResourceNotFound( ID );
	resources.add( ID );
}

void Resources::ResourceHandler_Read::inc_refCount( Utilities::GUID ID )noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		"Write to log";
	else
		resources.get<Entries::RefCount>( *find )++;
}

void Resources::ResourceHandler_Read::dec_refCount( Utilities::GUID ID )noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		"Write to log";
	else
		resources.get<Entries::RefCount>( *find )--;
}

Resources::RefCount Resources::ResourceHandler_Read::get_refCount( Utilities::GUID ID ) const noexcept
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		"Write to log";
	else
		return resources.peek<Entries::RefCount>( *find );
	return 0;
}

void Resources::ResourceHandler_Read::use_data( Utilities::GUID ID, const std::function<void( const Utilities::Memory::MemoryBlock )>& callback )
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		"Write to log";
	else
	{
		while ( !(resources.peek<Entries::State>( *find )& Resource_State::Pass0) )
			update();

		allocator( [&]( Utilities::Memory::ChunkyAllocator& a )
		{
			a.use_data( resources.peek<Entries::Handle>( *find ), callback );
		} );
	}
}

void Resources::ResourceHandler_Read::Pass0::entry( bool* running )noexcept
{
	while ( *running )
		if ( !to_load.isEmpty() )
		{
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
}
