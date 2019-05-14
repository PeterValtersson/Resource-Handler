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
			auto p = Resources::createResourceArchive( "test.dat", Resources::AccessMode::read_write );
		}
	};
}