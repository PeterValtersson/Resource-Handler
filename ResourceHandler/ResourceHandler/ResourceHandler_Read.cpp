#include "ResourceHandler_Read.h"
#include <Utilities/Profiler/Profiler.h>

template<typename T>
bool is_ready( const std::future<T>& f )
{
	return f.valid() && f.wait_for( std::chrono::milliseconds( 0 ) ) == std::future_status::ready;
}



using namespace std::chrono_literals;






Resources::ResourceHandler_Read::ResourceHandler_Read( std::shared_ptr<IResourceArchive> archive )
	: archive( archive ), allocator( 1_gb / Utilities::Memory::ChunkyAllocator::blocksize() ), loader( archive, allocator )
{
	using namespace std::placeholders;
	action_map[Action_Request::Type::Register_Resource] = std::bind( &ResourceHandler_Read::_register_resource, this, _1 );
	action_map[Action_Request::Type::Inc_RefCount] = std::bind( &ResourceHandler_Read::_inc_refCount, this, _1 );
	action_map[Action_Request::Type::Dec_RefCount] = std::bind( &ResourceHandler_Read::_dec_refCount, this, _1 );
	action_map[Action_Request::Type::Get_RefCount] = std::bind( &ResourceHandler_Read::_get_refCount, this, _1 );
	action_map[Action_Request::Type::Use_Data] = std::bind( &ResourceHandler_Read::_use_data, this, _1 );


	running = true;
	thread = std::thread( &ResourceHandler_Read::update, this );
	loader.start();
}

Resources::ResourceHandler_Read::~ResourceHandler_Read()
{
	running = false;
	if ( thread.joinable() )
		thread.join();
	loader.stop();
}

void Resources::ResourceHandler_Read::update()noexcept
{
	using namespace std::placeholders;
	PROFILE( "Resource Handler Thread" );
	while ( running )
	{
		PROFILE( "Running" );

		perform_actions();

		loader.update( std::bind( &ResourceHandler_Read::resource_loading_finished, this, _1, _2, _3 ),
					   std::bind( &ResourceHandler_Read::choose_resource_to_load, this ) );

		loader.run();
		std::this_thread::sleep_for( 32ms );
	}
}
void Resources::ResourceHandler_Read::resource_loading_finished( Utilities::GUID ID, Utilities::Memory::Handle handle, Status status )
{
	PROFILE;
	if ( auto find = resources.find( ID ); !find.has_value() )
		return; // Logg Resource no longer in use and has been unregistered.
	else
	{
		resources.get<Entries::Memory_Raw>( *find ) = handle;
		resources.get<Entries::Status>( *find ) |= status;
		resources.get<Entries::Status>( *find ) &= ~Status::Loading;
	}
}
Utilities::optional<Utilities::GUID> Resources::ResourceHandler_Read::choose_resource_to_load()
{
	PROFILE;
	for ( size_t i = 0; i < resources.size(); ++i )// TODO: Change to a suitable method for choosing resource to load  (FIFO, FILO, Highest refcount, etc.)
	{
		if ( !flag_has( resources.peek<Entries::Status>( i ), Status::In_Memory | Status::Could_Not_Load ) )
		{
			if ( archive->get_size( resources.peek<Entries::ID>( i ) ) > 0 )
			{
				resources.get<Entries::Status>( i ) |= Status::Loading;
				return resources.peek<Entries::ID>( i );
			}
		}
	}
	return std::nullopt;
}

void Resources::ResourceHandler_Read::register_resource( const Utilities::GUID ID )noexcept
{
	PROFILE;
	if ( !archive->exists( ID ) )
		return;

	action_request_queue.push( { Action_Request::Type::Register_Resource, ID } );
}

void Resources::ResourceHandler_Read::inc_refCount( const Utilities::GUID ID )noexcept
{
	PROFILE;
	action_request_queue.push( { Action_Request::Type::Inc_RefCount, ID } );
}

void Resources::ResourceHandler_Read::dec_refCount( const Utilities::GUID ID )noexcept
{
	PROFILE;
	action_request_queue.push( { Action_Request::Type::Dec_RefCount, ID } );
}

Resources::RefCount Resources::ResourceHandler_Read::get_refCount( const Utilities::GUID ID ) const noexcept
{
	PROFILE;
	std::promise<Action_Request::Response> p;
	auto f = p.get_future();
	action_request_queue.push( { Action_Request::Type::Get_RefCount, ID, std::move( p ) } );
	return f.get().get<RefCount>();
}

std::string Resources::ResourceHandler_Read::get_name( const Utilities::GUID ID ) const
{
	PROFILE;
	return archive->get_name( ID );
}

void Resources::ResourceHandler_Read::use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback )noexcept
{
	PROFILE;
	std::promise<Action_Request::Response> p;
	auto f = p.get_future();
	action_request_queue.push( { Action_Request::Type::Use_Data, ID, std::move( p ) } );

	try
	{
		const auto handle = f.get().get<Utilities::Memory::Handle>();
		allocator( [&]( const Utilities::Memory::ChunkyAllocator& a )
		{
			a.peek_data( handle, callback );
		} );
	}
	catch ( ... )
	{

	}
}

void Resources::ResourceHandler_Read::perform_actions()
{
	PROFILE;
	while ( !action_request_queue.isEmpty() )
	{
		auto& top = action_request_queue.top();
		if ( auto find = action_map.find( top.type ); find != action_map.end() )
		{
			try
			{
				top.promise.set_value( find->second( top.ID ) );
			}
			catch ( ... )
			{
				// Logg
			}
		}
		action_request_queue.pop();
	}
}
Resources::ResourceHandler_Read::Action_Request::Response Resources::ResourceHandler_Read::_register_resource( Utilities::GUID ID )
{
	PROFILE;
	if ( const auto find = resources.find( ID ); !find.has_value() )
		resources.add( ID, Status::None, 0, 0, 0 );
	return Action_Request::Response();
}
Resources::ResourceHandler_Read::Action_Request::Response Resources::ResourceHandler_Read::_inc_refCount( Utilities::GUID ID )
{
	PROFILE;
	if ( const auto find = resources.find( ID ); !find.has_value() )
		++resources.get<Entries::RefCount>( *find );
	return Action_Request::Response();
}
Resources::ResourceHandler_Read::Action_Request::Response Resources::ResourceHandler_Read::_dec_refCount( Utilities::GUID ID )
{
	PROFILE;
	if ( const auto find = resources.find( ID ); !find.has_value() )
		--resources.get<Entries::RefCount>( *find );
	return Action_Request::Response();
}
Resources::ResourceHandler_Read::Action_Request::Response Resources::ResourceHandler_Read::_get_refCount( Utilities::GUID ID )
{
	PROFILE;
	if ( const auto find = resources.find( ID ); !find.has_value() )
		return resources.peek<Entries::RefCount>( *find );
	return RefCount( 0 );
}
Resources::ResourceHandler_Read::Action_Request::Response Resources::ResourceHandler_Read::_use_data( Utilities::GUID ID )
{
	PROFILE;
	if ( const auto find = resources.find( ID ); !find.has_value() )
		throw ResourceNotFound( ID );
	else
	{
		if ( archive->get_size( ID ) == 0 )
			throw NoResourceData( archive->get_name( ID ), ID );
		else if ( flag_has( resources.peek<Entries::Status>( *find ), Status::In_Memory_Parsed ) )
			return resources.peek<Entries::Memory_Parsed>( *find );
		else if ( flag_has( resources.peek<Entries::Status>( *find ), Status::In_Memory ) )
			return resources.peek<Entries::Memory_Raw>( *find );
	}
	return Action_Request::Response();
}

void Resources::ResourceHandler_Read::Loader::start()noexcept
{
	to_load.load = false;
	running = true;
	//thread = std::thread(&Resources::ResourceHandler_Read::Loader::run, this);
}

void Resources::ResourceHandler_Read::Loader::stop()noexcept
{
	running = false;
	if ( thread.joinable() )
		thread.join();
}

void Resources::ResourceHandler_Read::Loader::update(
	std::function<void( Utilities::GUID ID, Utilities::Memory::Handle handle, Status status )> do_if_finished,
	std::function<Utilities::optional<Utilities::GUID>()> choose_to_load )noexcept
{
	PROFILE;
	if ( is_ready( to_load.future ) )
	{
		try
		{
			auto result = to_load.future.get();
			do_if_finished( to_load.ID, result, Status::In_Memory );
		}
		catch ( ResourceNotFound & e )
		{
			do_if_finished( to_load.ID, 0, Status::Not_Found );
		}
		catch ( Utilities::Memory::InvalidHandle & e )
		{
			do_if_finished( to_load.ID, 0, Status::Could_Not_Load );
		}
	}

	if ( !to_load.load )
	{
		if ( auto id = choose_to_load(); id.has_value() )
		{
			to_load.promise = decltype( to_load.promise )( );
			to_load.future = to_load.promise.get_future();
			to_load.ID = *id;
			to_load.load = true;
		}
	}
}

void Resources::ResourceHandler_Read::Loader::run()noexcept
{
	PROFILE_N( "Loader Thread" );
	//while (running)
	{
		PROFILE_N( "Running" );
		if ( to_load.load )
		{
			try
			{
				auto handle = allocator( [this]( Utilities::Memory::ChunkyAllocator& a )
				{
					return archive->read( to_load.ID, a );
				} );
				to_load.promise.set_value( handle );
			}
			catch ( ... )
			{
				to_load.promise.set_exception( std::current_exception() );
			}


			to_load.load = false;
		}
	}
}
