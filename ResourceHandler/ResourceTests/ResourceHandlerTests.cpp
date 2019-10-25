#include "stdafx.h"
#include "CppUnitTest.h"
#include <Resource.h>
#include <filesystem>
namespace fs = std::filesystem;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ResourceTests
{
	TEST_CLASS( ResourceTests )
	{
	public:

		TEST_METHOD( create )
		{
			auto rh = Resources::IResourceHandler::create( Resources::AccessMode::read_write, Resources::IResourceArchive::create_binary_archive( "test.dat", Resources::AccessMode::read_write ) );
			rh->get();
		}

		TEST_METHOD( Create_Resource_Read_Only )
		{
			auto rh = Resources::IResourceHandler::create( Resources::AccessMode::read_only, Resources::IResourceArchive::create_binary_archive( "test.dat", Resources::AccessMode::read_only ) );

			Assert::ExpectException<Resources::ResourceNotFound>( []
			{
				Resources::Resource r( "test" );
			});
		}

		TEST_METHOD( Read_Resource )
		{
			if ( fs::exists( "test.dat" ) )
				fs::remove( "test.dat" );
			{
				auto a = Resources::IResourceArchive::create_binary_archive( "test.dat", Resources::AccessMode::read_write );
				a->create( "test" );
				a->set_name( "test", "test" );
				a->set_type( "test", "test_type" );			
				Utilities::Memory::ChunkyAllocator all( 64 );
				auto handle = all.allocate( sizeof( int ) );
				all.use_data( handle, []( const Utilities::Memory::MemoryBlock mem )
				{
					mem = 1337;
				} );
				a->save_resource_info_data( { "test", handle }, all );
			}

			{
				auto rh = Resources::IResourceHandler::create( Resources::AccessMode::read_only, Resources::IResourceArchive::create_binary_archive( "test.dat", Resources::AccessMode::read_only ) );

				Resources::Resource r( "test" );
				r.use_data( []( const Utilities::Memory::MemoryBlock mem )
				{
					Assert::AreEqual( 1337, mem.peek<int>() );
				} );
			}
			
		}
	};
}