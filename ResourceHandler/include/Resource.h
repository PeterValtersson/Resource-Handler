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

		void check_in() noexcept
		{
			if ( !checkedIn )
				IResourceHandler::get()->inc_refCount( ID );
			checkedIn = true;
		}

		void check_out() noexcept
		{
			if ( checkedIn )
				IResourceHandler::get()->dec_refCount( ID );
			checkedIn = false;
		}

		uint32_t total_refCount()const noexcept
		{
			return IResourceHandler::get()->get_refCount( ID );
		}

		void use_data(const std::function<void(const Utilities::Memory::ConstMemoryBlock data)>& callback)
		{
			check_in();
			IResourceHandler::get()->use_data( ID, callback );
		}

		void write( const char* const data, size_t size )
		{
			check_in();
			IResourceHandler::get()->write_data( ID, data, size );
		}
		template<class T>
		void write( const T& t )
		{

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