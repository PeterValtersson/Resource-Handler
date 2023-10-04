#include "ResourceHandler_Read.h"
#include <Utilities/Profiler/Profiler.h>


using namespace std::chrono_literals;





ResourceHandler::ResourceHandler_Read::ResourceHandler_Read( std::shared_ptr<IResourceArchive> archive ) : 
	archive( archive ), allocator( 1_gb / Utilities::Memory::ChunkyAllocator::blocksize() ), 
	implementation( archive, allocator )
{
	using namespace std::placeholders;
	action_map[RequestType::Register_Resource] = [&]( Utilities::GUID ID )
	{
		implementation.register_resource( ID );
		return Request::Response();
	};
	action_map[RequestType::Get_Status] = [&]( Utilities::GUID ID )
	{
		return Request::Response( implementation.get_status( ID ) );
	};
	action_map[RequestType::Inc_RefCount] = [&]( Utilities::GUID ID )
	{
		implementation.inc_refCount( ID );
		return Request::Response();
	};
	action_map[RequestType::Dec_RefCount] = [&]( Utilities::GUID ID )
	{
		implementation.dec_refCount( ID );
		return Request::Response();
	};
	action_map[RequestType::Get_RefCount] = [&]( Utilities::GUID ID )
	{
		return Request::Response( implementation.get_refCount( ID ) );
	};
	action_map[RequestType::Use_Data] = [&]( Utilities::GUID ID )
	{
		return Request::Response( implementation.get_handle( ID ) );
	};


	running = true;
	thread = std::thread( &ResourceHandler_Read::update, this );
}

ResourceHandler::ResourceHandler_Read::~ResourceHandler_Read()
{
	running = false;
	if ( thread.joinable() )
		thread.join();
}

void ResourceHandler::ResourceHandler_Read::update()noexcept
{

	PROFILE( "Resource Handler Thread" );
	while ( running )
	{

		perform_actions();

		implementation.run();
		std::this_thread::sleep_for( 32ms );
	}
}
void ResourceHandler::ResourceHandler_Read::set_parser_memory_handler(std::shared_ptr<Utilities::Memory::Allocator> memory_handler)
{
}
std::shared_ptr<ResourceHandler::IResourceArchive> ResourceHandler::ResourceHandler_Read::get_archive()
{
	return archive;
}
void ResourceHandler::ResourceHandler_Read::add_parser(const Utilities::GUID type, const std::string& library_path)
{
}
void ResourceHandler::ResourceHandler_Read::add_parser(const Utilities::GUID type, const parse_callback& parse_callback)
{
}
void ResourceHandler::ResourceHandler_Read::register_resource( const Utilities::GUID ID, const Flags flag)noexcept
{
	PROFILE;
	if ( !archive->exists( ID ) )
		return;

	action_request_queue.push( { RequestType::Register_Resource, ID } );
}

ResourceHandler::Status ResourceHandler::ResourceHandler_Read::get_status( const Utilities::GUID ID ) noexcept
{
	std::promise<Request::Response> p;
	auto f = p.get_future();
	action_request_queue.push( { RequestType::Get_Status, ID, std::move( p ) } );
	return f.get().get<Status>();
}

void ResourceHandler::ResourceHandler_Read::set_flag(const Utilities::GUID ID, const Flags flag) noexcept
{
}

void ResourceHandler::ResourceHandler_Read::remove_flag(const Utilities::GUID ID, const Flags flag) noexcept
{
}

void ResourceHandler::ResourceHandler_Read::inc_refCount( const Utilities::GUID ID )noexcept
{
	PROFILE;
	action_request_queue.push( { RequestType::Inc_RefCount, ID } );
}

void ResourceHandler::ResourceHandler_Read::dec_refCount( const Utilities::GUID ID )noexcept
{
	PROFILE;
	action_request_queue.push( { RequestType::Dec_RefCount, ID } );
}

ResourceHandler::RefCount ResourceHandler::ResourceHandler_Read::get_refCount( const Utilities::GUID ID ) const noexcept
{
	PROFILE;
	std::promise<Request::Response> p;
	auto f = p.get_future();
	action_request_queue.push( { RequestType::Get_RefCount, ID, std::move( p ) } );
	return f.get().get<RefCount>();
}

std::string ResourceHandler::ResourceHandler_Read::get_name( const Utilities::GUID ID ) const
{
	PROFILE;
	return archive->get_name( ID );
}

void ResourceHandler::ResourceHandler_Read::use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback )
{
	PROFILE;
	std::promise<Request::Response> p;
	auto f = p.get_future();
	action_request_queue.push( { RequestType::Use_Data, ID, std::move( p ) } );


	const auto handle = f.get().get<Utilities::Memory::Handle>();
	allocator( [&]( const Utilities::Memory::ChunkyAllocator& a )
	{
		a.peek_data( handle, callback );
	} );

}

void ResourceHandler::ResourceHandler_Read::perform_actions()
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
				top.promise.set_exception( std::current_exception() );
			}
		}
		action_request_queue.pop();
	}
}
