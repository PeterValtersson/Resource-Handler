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

		TEST_METHOD( ResourceTests_InitResourceHandler )
		{
			std::vector<std::unique_ptr<Resources::IResourceArchive>> archives;
			archives.push_back( Resources::createResourceArchive( "test.dat", Resources::AccessMode::read_write ) );
			auto rh = Resources::IResourceHandler::create( Resources::AccessMode::read_write, archives );
		}
	
	};
}