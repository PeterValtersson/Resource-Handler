#ifndef _RESOURCES_RESOURCE_H_
#define _RESOURCES_RESOURCE_H_

#include <GUID.h>
#include <IResourceHandler.h>

namespace Resources
{
	
	class Resource_Base {
	public:
		Resource_Base( Utilities::GUID ID )noexcept : ID( ID ), checkedIn( false ), memory( {nullptr} )
		{
			IResourceHandler::get()->registerResource( this );
		}
		Resource_Base( const Resource_Base& other ) = delete;
		Resource_Base( Resource_Base&& other ) = delete;
		Resource_Base& operator=( const Resource_Base& other ) = delete;
		Resource_Base& operator=( Resource_Base&& other ) = delete;

		~Resource_Base()noexcept { }

		bool operator==( const Resource_Base& other )const noexcept
		{
			return ID == other.ID;
		}

		void checkIn() noexcept
		{
			if ( !checkedIn )
				IResourceHandler::get()->refCountInc( this );
			checkedIn = true;
		}

		void checkOut() noexcept
		{
			if ( checkedIn )
			{
				IResourceHandler::get()->refCountDec( this );
				memory.data = nullptr;
				checkedIn = false;
			}
		}

		uint32_t totalRefCount()const noexcept
		{
			return IResourceHandler::get()->getRefCount( this );		
		}

	protected:
		Utilities::GUID ID;
		bool checkedIn;
		Memory_Block memory;

		void loadData()
		{
			checkIn();
			if ( !memory.data )
			{
				try
				{
					memory = IResourceHandler::get()->getResourceData( this );
				}
				catch ( ... )
				{
					checkOut();
					throw;
				}
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
		inline operator const T&() { return get(); }
		inline const T& operator*() { return get(); }
	};
}

#endif