#include "stdafx.h"
#include "CppUnitTest.h"
//#include <Resource.h>
//#include <IResourceHandler.h>
#include <IResourceArchive.h>
#include <filesystem>
namespace fs = std::experimental::filesystem;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ResourceTests
{
	TEST_CLASS( ResourceArchiveTests )
	{
	public:

		TEST_METHOD( ResourceArchiveTests_InitTest )
		{
			ResourceArchive::createResourceArchive( "test.data", ResourceArchive::ArchiveMode::development );
		}
		TEST_METHOD( ResourceArchiveTests_WriteInRuntime )
		{
			auto resourceArchive = ResourceArchive::createResourceArchive( "test.data", ResourceArchive::ArchiveMode::runtime );
			Assert::ExpectException<ResourceArchive::ArchiveNotInDeveloperMode>( [&]
			{
				resourceArchive->write( "TestFile", nullptr, 0 ); return 0;
			} );
		}
		TEST_METHOD( ResourceArchiveTests_ReadFileNotFound )
		{
			auto resourceArchive = ResourceArchive::createResourceArchive( "test.data", ResourceArchive::ArchiveMode::runtime );
			Assert::ExpectException<ResourceArchive::ArchiveResourceNotFound>( [&]
			{
				resourceArchive->read( "This file should not exist", []( auto data ) { } ); return 0;
			} );
			Assert::ExpectException<ResourceArchive::ArchiveResourceNotFound>( [&]
			{
				resourceArchive->getSize( "This file should not exist" ); return 0;
			} );
			Assert::IsFalse(resourceArchive->exists( "This file should not exist" ));
		}
		TEST_METHOD( ResourceArchiveTests_ArchiveNotFound )
		{
			Assert::ExpectException<ResourceArchive::ArchivePathNotFound>( []
			{
				ResourceArchive::createResourceArchive( "this file should not exist", ResourceArchive::ArchiveMode::runtime ); return 0;
			} );
		}

		TEST_METHOD( ResourceArchiveTests_WriteRead )
		{
			if (fs::exists("test.data") )
				fs::remove( "test.data" );

			int testint = 1337;
			{
				auto resourceArchive = ResourceArchive::createResourceArchive( "test.data", ResourceArchive::ArchiveMode::development );

				
				resourceArchive->write( "TestFile", &testint, sizeof(testint) );
				resourceArchive->setName( "TestFile", "TestFile" );
				Assert::IsTrue( resourceArchive->exists( "TestFile" ), L"File was not created" );
				Assert::AreEqual( sizeof( testint ), resourceArchive->getSize( "TestFile" ), L"File wrong size" );
				Assert::AreEqual( "TestFile", resourceArchive->getName( "TestFile" ).c_str(), L"File wrong name" );

				resourceArchive->read( "TestFile", [&]( auto data )
				{
					Assert::AreEqual( testint, *(int*)data.data, L"Data not correct" );
				} );

				resourceArchive->save();
			}
			{
				auto resourceArchive = ResourceArchive::createResourceArchive( "test.data", ResourceArchive::ArchiveMode::runtime );

			
				Assert::IsTrue( resourceArchive->exists( "TestFile" ), L"File was not read from file" );
				Assert::AreEqual( sizeof( testint ), resourceArchive->getSize( "TestFile" ), L"File wrong size" );
				Assert::AreEqual( "TestFile", resourceArchive->getName( "TestFile" ).c_str(), L"File wrong name" );

				resourceArchive->read( "TestFile", [&]( auto data )
				{
					Assert::AreEqual( testint, *(int*)data.data, L"Data not correct" );
				} );

				
			}
		}
	};
}