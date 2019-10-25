#ifndef _RESOURCES_RESOURCE_H_
#define _RESOURCES_RESOURCE_H_

#include <Utilities/GUID.h>
#include <functional>
#include <IResourceHandler.h>

namespace Resources
{
	class Resource {
	public:
		Resource( Utilities::GUID ID ) : ID(ID), checkedIn(false)
		{
			IResourceHandler::get()->register_resource( ID );
		}
		Resource( const Resource& other ) = delete;
		Resource( Resource&& other ) = delete;
		Resource& operator=( const Resource& other ) = delete;
		Resource& operator=( Resource&& other ) = delete;

		~Resource()
		{ 
			check_out();
		}

		bool operator==( const Resource& other )const noexcept
		{
			return ID == other.ID;
		}

		void check_in() 
		{
			if ( !checkedIn )
				IResourceHandler::get()->inc_refCount( ID );
			checkedIn = true;
		}

		void check_out() 
		{
			if ( checkedIn )
				IResourceHandler::get()->dec_refCount( ID );
			checkedIn = false;
		}

		uint32_t total_refCount()const
		{
			return IResourceHandler::get()->get_refCount( ID );
		}

		void use_data(const std::function<void(const Utilities::Memory::MemoryBlock data)>& callback)
		{
			check_in();
			IResourceHandler::get()->use_data( ID, callback );
		}

	protected:
		Utilities::GUID ID;
		bool checkedIn;
	};
	/*
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
	};*/
}

#endif