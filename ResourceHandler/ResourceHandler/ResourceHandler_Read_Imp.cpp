#include "ResourceHandler_Read_Imp.h"
#include <Utilities/Profiler/Profiler.h>

using namespace std::placeholders;

ResourceHandler::ResourceHandler_Read_Imp::ResourceHandler_Read_Imp( std::shared_ptr<IResourceArchive> archive, Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator ) :
	archive( archive ),
	allocator(allocator),
	loader( archive, allocator )
{
	loader.start();
}

ResourceHandler::ResourceHandler_Read_Imp::~ResourceHandler_Read_Imp()
{
	loader.stop();
}

void ResourceHandler::ResourceHandler_Read_Imp::run()
{
	loader.update( std::bind( &ResourceHandler_Read_Imp::resource_loading_finished, this, _1, _2, _3 ),
				   std::bind( &ResourceHandler_Read_Imp::choose_resource_to_load, this ) );

	loader.run();
}

void ResourceHandler::ResourceHandler_Read_Imp::register_resource( Utilities::GUID ID )noexcept
{
	PROFILE;
	if ( const auto find = resources.find( ID ); !find.has_value() )
		resources.add( ID, Status::None, 0, 0, 0 );
}
ResourceHandler::Status ResourceHandler::ResourceHandler_Read_Imp::get_status( Utilities::GUID ID )noexcept
{
	if ( const auto find = resources.find( ID ); find.has_value() )
		return resources.peek<Entries::Status>( *find );
	return Status::Not_Found;
}
void ResourceHandler::ResourceHandler_Read_Imp::inc_refCount( Utilities::GUID ID )noexcept
{
	PROFILE;
	if ( const auto find = resources.find( ID ); find.has_value() )
		++resources.get<Entries::RefCount>( *find );
}
void ResourceHandler::ResourceHandler_Read_Imp::dec_refCount( Utilities::GUID ID )noexcept
{
	PROFILE;
	if ( const auto find = resources.find( ID ); find.has_value() )
		--resources.get<Entries::RefCount>( *find );
}
ResourceHandler::RefCount ResourceHandler::ResourceHandler_Read_Imp::get_refCount( Utilities::GUID ID )noexcept
{
	PROFILE;
	if ( const auto find = resources.find( ID ); find.has_value() )
		return resources.peek<Entries::RefCount>( *find );
	return RefCount( 0 );
}
Utilities::Memory::Handle ResourceHandler::ResourceHandler_Read_Imp::get_handle( Utilities::GUID ID )
{
	PROFILE;
	if ( const auto find = resources.find( ID ); !find.has_value() )
		throw ResourceNotFound( ID );
	else
	{
		while ( true )
		{
			if ( archive->get_size( ID ) == 0 )
				throw NoResourceData( archive->get_name( ID ), ID );
			else if ( flag_has( resources.peek<Entries::Status>( *find ), Status::In_Memory ) )
				return resources.peek<Entries::Memory_Raw>( *find );
			else if ( flag_has( resources.peek<Entries::Status>( *find ), Status::Could_Not_Load ) )
			{
				throw Utilities::Memory::InvalidHandle( "" );
			}
			else if ( flag_has( resources.peek<Entries::Status>( *find ), Status::Not_Found ) )
			{
				throw ResourceNotFound( ID );
			}
			else if ( flag_has( resources.peek<Entries::Status>( *find ), Status::Loading ) )
			{
				loader.update( std::bind( &ResourceHandler_Read_Imp::resource_loading_finished, this, _1, _2, _3 ),
							   std::bind( &ResourceHandler_Read_Imp::choose_resource_to_load, this ) );

				loader.run();
			}
			else
			{
				loader.update( std::bind( &ResourceHandler_Read_Imp::resource_loading_finished, this, _1, _2, _3 ),
							   [&]()
				{
					return ID;
				} );
				loader.run();
			}
		}
	}
}


void ResourceHandler::ResourceHandler_Read_Imp::resource_loading_finished( Utilities::GUID ID, Utilities::Memory::Handle handle, Status status )
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
Utilities::optional<Utilities::GUID> ResourceHandler::ResourceHandler_Read_Imp::choose_resource_to_load()
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




void ResourceHandler::ResourceHandler_Read_Imp::Loader::start()noexcept
{
	to_load.load = false;
	running = true;
	//thread = std::thread(&ResourceHandler::ResourceHandler_Read::Loader::run, this);
}

void ResourceHandler::ResourceHandler_Read_Imp::Loader::stop()noexcept
{
	running = false;
	if ( thread.joinable() )
		thread.join();
}



template<typename T>
bool is_ready( const std::future<T>& f )
{
	return f.valid() && f.wait_for( std::chrono::milliseconds( 0 ) ) == std::future_status::ready;
}



void ResourceHandler::ResourceHandler_Read_Imp::Loader::update(
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
		catch ( ResourceNotFound& e )
		{
			do_if_finished( to_load.ID, 0, Status::Not_Found );
		}
		catch ( Utilities::Memory::InvalidHandle& e )
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

void ResourceHandler::ResourceHandler_Read_Imp::Loader::run()noexcept
{
	//PROFILE_N( "Loader Thread" );
	//while (running)
	{
		PROFILE;
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
