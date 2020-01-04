#include "stdafx.h"
#include "CppUnitTest.h"
#include <IResourceArchive.h>
#include <filesystem>
namespace fs = std::filesystem;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ResourceTests
{
	TEST_CLASS( ZipArchiveTests ) {
public:
	TEST_METHOD( create_read_only )
	{
		Assert::ExpectException<Resources::PathNotFound>( [&]
		{
			Resources::IResourceArchive::create_zip_archive( "shouldnotexist.dat", Resources::AccessMode::read );
		} );

	}
	TEST_METHOD( create )
	{
		if ( fs::exists( "test.dat" ) )
			fs::remove( "test.dat" );
		auto a = Resources::IResourceArchive::create_zip_archive( "test.dat", Resources::AccessMode::read_write );
		Assert::IsTrue( fs::exists( "test.dat" ), L"'test.dat' not created" );
		Assert::AreEqual( 0ui64, a->num_resources(), L"0 != num_resources()" );
	}
	TEST_METHOD( get_non_exist )
	{
		if ( fs::exists( "test.dat" ) )
			fs::remove( "test.dat" );
		auto a = Resources::IResourceArchive::create_zip_archive( "test.dat", Resources::AccessMode::read_write );
		Assert::ExpectException<Resources::ResourceNotFound>( [&]
		{
			a->get_name( "test" );
		}, L"get_name" );

		Assert::ExpectException<Resources::ResourceNotFound>( [&]
		{
			a->get_size( "test" );
		}, L"get_size" );

	}

	TEST_METHOD( create_resource )
	{
		if ( fs::exists( "test.dat" ) )
			fs::remove( "test.dat" );
		{
			auto a = Resources::IResourceArchive::create_zip_archive( "test.dat", Resources::AccessMode::read_write );
			Assert::IsFalse( a->exists( "test" ) );
			Assert::ExpectException<Resources::ResourceNotFound>( [&]
			{
				a->set_name( "test", "test" );
			} );

			a->create( "test" );
			Assert::IsTrue( a->exists( "test" ) );
			a->set_name( "test", "test" );

			Assert::AreEqual<std::string>( "test", a->get_name( "test" ) );

			Utilities::Memory::ChunkyAllocator all( 64 );
			auto handle = all.allocate( sizeof( int ) );
			all.use_data( handle, []( Utilities::Memory::MemoryBlock mem )
			{
				mem = 1337;
			} );

			a->save( { "test", handle }, all );
		}

		{
			auto a = Resources::IResourceArchive::create_zip_archive( "test.dat", Resources::AccessMode::read_write );
			Assert::IsTrue( a->exists( "test" ) );
			Assert::AreEqual<std::string>( "test", a->get_name( "test" ) );
			
			Utilities::Memory::ChunkyAllocator all( 64 );
			auto handle = a->read( "test", all );
			all.use_data( handle, []( const Utilities::Memory::MemoryBlock mem )
			{
				Assert::AreEqual( 1337, mem.peek<int>() );
			} );
		}
	}
	};
}