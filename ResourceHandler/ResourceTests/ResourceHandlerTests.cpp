#include "stdafx.h"
#include "CppUnitTest.h"
#include <Resource.h>
//#include <IResourceHandler.h>

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
	};
}