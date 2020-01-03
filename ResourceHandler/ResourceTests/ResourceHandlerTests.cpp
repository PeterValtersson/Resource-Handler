#include "stdafx.h"
#include "CppUnitTest.h"
#include <Resource.h>
#include <filesystem>
namespace fs = std::filesystem;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ResourceTests
{
	TEST_CLASS( ResourceTests ){
public:

	TEST_METHOD( create )
	{
		Resources::IResourceHandler::create( Resources::AccessMode::read_write, Resources::IResourceArchive::create_binary_archive( "test.dat", Resources::AccessMode::read_write ) );
		Resources::IResourceHandler::get();
	}

	TEST_METHOD( Create_Resource_Read_Only )
	{
		Resources::IResourceHandler::create( Resources::AccessMode::read, Resources::IResourceArchive::create_binary_archive( "test.dat", Resources::AccessMode::read ) );

		Assert::ExpectException<Resources::ResourceNotFound>( []
		{
			Resources::Resource r( "test" );
		} );
	}

	TEST_METHOD( Read_Resource )
	{
		if ( fs::exists( "test2.dat" ) )
			fs::remove( "test2.dat" );
		{
			auto a = Resources::IResourceArchive::create_binary_archive( "test2.dat", Resources::AccessMode::read_write );
			a->create( "test" );
			a->set_name( "test", "test" );
			a->set_type( "test", "test_type" );
			Utilities::Memory::ChunkyAllocator all( 64 );
			auto handle = all.allocate( sizeof( int ) );
			all.use_data( handle, []( Utilities::Memory::MemoryBlock mem )
			{
				mem = 1337;
			} );
			a->save_resource_info_data( { "test", handle }, all );
		}

		{
			Resources::IResourceHandler::create( Resources::AccessMode::read, Resources::IResourceArchive::create_binary_archive( "test2.dat", Resources::AccessMode::read ) );

			Resources::Resource r( "test" );
			r.use_data( []( const Utilities::Memory::ConstMemoryBlock mem )
			{
				Assert::AreEqual( 1337, mem.peek<int>() );
			} );
		}

	}
	};
}