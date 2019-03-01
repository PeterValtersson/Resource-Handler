#include "stdafx.h"
#include "CppUnitTest.h"
//#include <Resource.h>
//#include <IResourceHandler.h>
#include <IResourceArchive.h>

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
			ResourceArchive::Memory_Block mem;
			Assert::ExpectException<ResourceArchive::ArchiveNotInDeveloperMode>( [&]
			{
				resourceArchive->write( "TestFile", mem ); return 0;
			} );
		}
		TEST_METHOD( ResourceArchiveTests_ArchiveNotFound )
		{
			Assert::ExpectException<ResourceArchive::ArchivePathNotFound>( []
			{
				ResourceArchive::createResourceArchive( "this file should not exist", ResourceArchive::ArchiveMode::runtime ); return 0;
			} );
		}
	};
}