#include "stdafx.h"
#include "CppUnitTest.h"
#include <Resource.h>
#include <IResourceHandler.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ResourceTests
{
	TEST_CLASS( UnitTest1 )
	{
	public:

		TEST_METHOD( SimpleCheckInOut )
		{
			Resources::Resource resource( "MyResource.txt" );
			Assert::AreEqual( 0u, resource.totalRefCount() );
			resource.checkIn();
			Assert::AreEqual( 1u, resource.totalRefCount() );
			resource.checkOut();
			Assert::AreEqual( 0u, resource.totalRefCount() );
		}

	};
}