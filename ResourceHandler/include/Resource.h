#ifndef _RESOURCES_RESOURCE_H_
#define _RESOURCES_RESOURCE_H_

#include <Utilities/GUID.h>
#include <functional>
#include <IResourceHandler.h>

namespace ResourceHandler
{
	class Resource{
	public:
		Resource() : ID(), checkedIn( false )
		{

		}
		Resource( Utilities::GUID ID, Flags flags = Flags::None ) : ID( ID ), checkedIn( false )
		{
			IResourceHandler::get()->register_resource( ID );
		}

		Resource( const Resource& other )noexcept : ID( other.ID ), checkedIn( false )
		{
			// Already registered when other was created.
		}

		Resource( Resource&& other )noexcept : ID( other.ID ), checkedIn( other.checkedIn )
		{
			// Already registered when other was created.
			other.ID = Utilities::GUID();
			other.checkedIn = false;
		}
		Resource& operator=( const Resource& other )noexcept
		{
			check_out(); // May already contain a resource
			ID = other.ID;
			return *this;
		}
		Resource& operator=( Resource&& other )noexcept
		{
			check_out(); // May already contain a resource
			ID = other.ID;
			checkedIn = other.checkedIn;
			other.ID = Utilities::GUID();
			other.checkedIn = false;
			return *this;
		}

		~Resource()
		{
			check_out();
		}

		inline bool operator==( const Resource& other )const noexcept
		{
			return ID == other.ID;
		}

		inline void check_in()
		{
			if ( !checkedIn )
				IResourceHandler::get()->inc_refCount( ID );
			checkedIn = true;
		}

		inline void check_out()
		{
			if ( checkedIn )
				IResourceHandler::get()->dec_refCount( ID );
			checkedIn = false;
		}
		inline void set_name( std::string_view name )
		{
			IResourceHandler::get()->set_name( ID, name );
		}
		inline std::string get_name()
		{
			return IResourceHandler::get()->get_name( ID );
		}
		inline uint32_t get_refCount()const
		{
			return IResourceHandler::get()->get_refCount( ID );
		}

		inline void use_data( const std::function<void( const Utilities::Memory::ConstMemoryBlock data )>& callback )const
		{
			if ( !checkedIn )
				IResourceHandler::get()->inc_refCount( ID );
			checkedIn = true;
			IResourceHandler::get()->use_data( ID, callback );
		}
		inline void modify_data( const std::function<void( const Utilities::Memory::MemoryBlock data )>& callback )
		{
			check_in();
			IResourceHandler::get()->modify_data( ID, callback );
		}
		

		template<class T>
		inline T get_copy()const
		{
			if ( !checkedIn )
				IResourceHandler::get()->inc_refCount( ID );
			checkedIn = true;
			T data_ret;
			IResourceHandler::get()->use_data( ID, [&]( const Utilities::Memory::ConstMemoryBlock data )
			{
				data_ret = data.peek<T>();
			} );
			return data_ret;
		}
		inline void write( const void* const data, size_t size )
		{
			check_in();
			IResourceHandler::get()->write_data( ID, data, size );
		}
		template<class T>
		inline void write( const T& t )
		{
			write( &t, sizeof( T ) );
		}

		Status get_status()const
		{
			return IResourceHandler::get()->get_status( ID );
		}

		operator Utilities::GUID()const
		{
			return ID;
		}

		void set_flag(const Flags flags)
		{
			IResourceHandler::get()->set_flag(ID, flags);
		}
		void remove_flag(const Flags flags)
		{
			IResourceHandler::get()->remove_flag(ID, flags);
		}
	protected:
		Utilities::GUID ID;
		mutable bool checkedIn;
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