#include "stdafx.h"
#include "CppUnitTest.h"
#include <Resource.h>
#include <stdint.h>
#include <filesystem>
namespace fs = std::filesystem;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ResourceTests
{
	struct MoreData {
		int a;
		float b;
		unsigned int c;
	};
	TEST_CLASS( ResourceTests ) {
	public:

		TEST_METHOD( create_not_set )
		{
			if ( fs::exists( "test.dat" ) )
				fs::remove( "test.dat" );
			Resources::IResourceHandler::create( Resources::AccessMode::read_write, Resources::IResourceArchive::create_zip_archive( "test.dat", Resources::AccessMode::read_write ) );
			Assert::ExpectException<Resources::NoResourceHandler>( []()
			{
				Resources::IResourceHandler::get();
			} );
		}
		TEST_METHOD( create )
		{
			if ( fs::exists( "test.dat" ) )
				fs::remove( "test.dat" );
			auto rh = Resources::IResourceHandler::create( Resources::AccessMode::read_write, Resources::IResourceArchive::create_zip_archive( "test.dat", Resources::AccessMode::read_write ) );
			Resources::IResourceHandler::set( rh );
			Resources::IResourceHandler::get();

		}
		TEST_METHOD( Read_Resources )
		{
			if ( fs::exists( "test2.dat" ) )
				fs::remove( "test2.dat" );
			{
				Utilities::Memory::ChunkyAllocator all( 64 );
				auto a = Resources::IResourceArchive::create_zip_archive( "test2.dat", Resources::AccessMode::read_write );
				a->create_from_name( "test" );
				a->set_name( "test", "test" );
				a->set_type( "test", "test_type" );
				auto handle = all.allocate( sizeof( int ) );
				all.use_data( handle, []( Utilities::Memory::MemoryBlock mem )
				{
					mem = 1337;
				} );

				a->create_from_name( "test2" );
				a->set_type( "test2", "test_type" );
				auto handle2 = all.allocate( sizeof( int ) );
				all.use_data( handle2, []( Utilities::Memory::MemoryBlock mem )
				{
					mem = 1338;
				} );

				a->create_from_name( "test3" );
				a->set_type( "test2", "test_type" );
				auto handle3 = all.allocate( sizeof( MoreData ) );
				all.use_data( handle3, []( Utilities::Memory::MemoryBlock mem )
				{
					mem = MoreData{ 123, 1.2f, 134 };
				} );

				Resources::To_Save_Vector to_save = { { "test", handle }, { "test2", handle2 }, { "test3", handle3 } };
				a->save_multiple( to_save, all );
			}

			{
				auto a = Resources::IResourceArchive::create_zip_archive( "test2.dat", Resources::AccessMode::read );
				auto rh = Resources::IResourceHandler::create( Resources::AccessMode::read, a );
				Resources::IResourceHandler::set( rh );

				Resources::Resource r( "test" );
				r.use_data( []( const Utilities::Memory::ConstMemoryBlock mem )
				{
					Assert::AreEqual( 1337, mem.peek<int>() );
				} );

				Resources::Resource r2( "test2" );
				r2.use_data( []( const Utilities::Memory::ConstMemoryBlock mem )
				{
					Assert::AreEqual( 1338, mem.peek<int>() );
				} );

				Resources::Resource r3( "test3" );
				r3.use_data( []( const Utilities::Memory::ConstMemoryBlock mem )
				{
					auto a = mem.peek<MoreData>();
					Assert::AreEqual( 123, mem.peek<MoreData>().a );
					Assert::AreEqual( 1.2f, mem.peek<MoreData>().b );
					Assert::AreEqual<unsigned int>( 134, mem.peek<MoreData>().c );
				} );
			}

		}
	};

	TEST_CLASS( ResourceTests_Write ) {
	public:

		TEST_METHOD( write_resource )
		{
			if ( fs::exists( "test.dat" ) )
				fs::remove( "test.dat" );
			auto a = Resources::IResourceArchive::create_zip_archive( "test.dat", Resources::AccessMode::read_write );
			auto rh = Resources::IResourceHandler::create( Resources::AccessMode::read_write, a );
			Resources::IResourceHandler::set( rh );

			Resources::Resource r( "test" );
			Assert::IsFalse( a->exists( "test" ) );

			r.set_name( "resource" );

			Assert::IsTrue( a->exists( "test" ) );
			Assert::AreEqual( "resource", a->get_name( "test" ).c_str() );
			Assert::AreEqual( "resource", r.get_name().c_str() );

			r.write( 123 );
			r.use_data( []( const Utilities::Memory::ConstMemoryBlock data )
			{
				Assert::AreEqual( 123, data.peek<int>() );
			} );

			
			r.use_data( []( const Utilities::Memory::ConstMemoryBlock data )
			{
				Assert::AreEqual( 123, data.peek<int>() );
				Assert::AreEqual( sizeof( int ), data.used_size );
			} );

			Assert::AreEqual<size_t>( 0, a->get_size( "test" ) );
			rh->save_all();

			Assert::AreEqual<size_t>( sizeof(int), a->get_size( "test" ) );
			Utilities::Memory::ChunkyAllocator all( 64 );
			auto h = a->read( "test", all );
			all.peek_data( h, []( const Utilities::Memory::ConstMemoryBlock data )
			{
				Assert::AreEqual( 123, data.peek<int>() );
			} );


			double test = 123.123;
			r.write( &test, sizeof( test ) );
			r.use_data( []( const Utilities::Memory::ConstMemoryBlock data )
			{
				Assert::AreEqual( 123.123, data.peek<double>() );
			} );
			Assert::AreEqual( sizeof( int ), a->get_size( "test" ) );
			r.use_data( []( const Utilities::Memory::ConstMemoryBlock data )
			{
				Assert::AreEqual( 123.123, data.peek<double>() );
				Assert::AreEqual( sizeof( double ), data.used_size );
			} );

			h = a->read( "test", all );
			all.peek_data( h, []( const Utilities::Memory::ConstMemoryBlock data )
			{
				Assert::AreEqual( 123, data.peek<int>() );
			} );

			rh->save_all();
			Assert::AreEqual( sizeof( double ), a->get_size( "test" ) );
			h = a->read( "test", all );
			all.peek_data( h, []( const Utilities::Memory::ConstMemoryBlock data )
			{
				Assert::AreEqual( 123.123, data.peek<double>() );
			} );
		}
		TEST_METHOD( modify_resource )
		{
			if ( fs::exists( "test.dat" ) )
				fs::remove( "test.dat" );
			auto rh = Resources::IResourceHandler::create( Resources::AccessMode::read_write, Resources::IResourceArchive::create_zip_archive( "test.dat", Resources::AccessMode::read_write ) );
			Resources::IResourceHandler::set( rh );

			Resources::Resource r( "test" );

			r.modify_data( []( Utilities::Memory::MemoryBlock data )
			{
				data = 1337;
			} );
			r.use_data( []( const Utilities::Memory::ConstMemoryBlock data )
			{
				Assert::AreEqual( 1337, data.peek<int>() );
			} );

			r.modify_data( []( Utilities::Memory::MemoryBlock data )
			{
				data = 123.123;
			} );
			r.use_data( []( const Utilities::Memory::ConstMemoryBlock data )
			{
				Assert::AreEqual( 123.123, data.peek<double>() );
			} );
		}
	};
}