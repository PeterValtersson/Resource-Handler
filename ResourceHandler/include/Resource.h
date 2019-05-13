#ifndef _RESOURCES_RESOURCE_H_
#define _RESOURCES_RESOURCE_H_

#include <GUID.h>
#include <functional>


namespace Resources
{
	
	class Resource_Base {
	public:
		Resource_Base( Utilities::GUID ID );
		Resource_Base( const Resource_Base& other ) = delete;
		Resource_Base( Resource_Base&& other ) = delete;
		Resource_Base& operator=( const Resource_Base& other ) = delete;
		Resource_Base& operator=( Resource_Base&& other ) = delete;

		~Resource_Base()
		{ 
			checkOut();
		}

		bool operator==( const Resource_Base& other )const noexcept
		{
			return ID == other.ID;
		}

		void checkIn() 
		{
			if ( !checkedIn )
				IResourceHandler::get()->checkIn( ID );
			checkedIn = true;
		}

		void checkOut() 
		{
			if ( checkedIn )
				IResourceHandler::get()->checkOut( ID );
			checkedIn = false;
		}

		uint32_t totalRefCount()const
		{
			return IResourceHandler::get()->getRefCount( ID );
		}

		Utilities::Allocators::ChunkyAllocator::ChunkyData data()
		{
			checkIn();
			return IResourceHandler::get()->getResourceData( ID );
		}

	protected:
		Utilities::GUID ID;
		bool checkedIn;
		ResourceArchive::ArchiveEntry data;
		void loadData()
		{
			checkIn();
			if ( !data.isValid() )
			{
				data = IResourceHandler::get()->getResourceData( ID );
			}
		}
	};

	template<class T>
	class Resource : public Resource_Base {
	public:
		Resource( Utilities::GUID ID ) : Resource_Base( ID ) { }
		inline const T* operator->()
		{	
			loadData();
			return (T*)memory.data;
		}
		inline const T& get()
		{
			loadData();
			return *(T*)memory.data;
		}
		inline void edit( const std::function<void( T& )>& callback )
		{
			loadData();
			callback( *(T*)memory.data );
		}
		inline operator const T&() { return get(); }
		inline const T& operator*() { return get(); }
	};
}

#endif