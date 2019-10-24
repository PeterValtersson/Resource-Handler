#ifndef _RESOURCE_ARCHIVE_H_
#define _RESOURCE_ARCHIVE_H_
#include <Utilities/GUID.h>
#include <Utilities/ErrorHandling.h>
#include <stdint.h>
#include <DLL_Export.h>
#include <Utilities/Memory/ChunkyAllocator.h>
#include <memory>
#include <functional>

namespace Resources
{
	struct PathNotFound : public Utilities::Exception{
		PathNotFound( std::string_view path ) : Utilities::Exception( "Path could not be found. Path: " + std::string(path) )
		{}
	};
	struct WriteInReadOnly : public Utilities::Exception{
		WriteInReadOnly(  ) : Utilities::Exception( "Tried to write when in read only mode" )
		{}
	};
	struct ResourceNotFound : public Utilities::Exception{
		ResourceNotFound( Utilities::GUID ID ) : Utilities::Exception( "Could not find resource. GUID: " + std::to_string( ID ) )
		{}
	};

	struct PathNotAccessible : public Utilities::Exception{
		PathNotAccessible( std::string_view path ) : Utilities::Exception( "Could not access path. Path: " + std::string( path ) )
		{}
	};

	struct ArchiveCorrupt : public Utilities::Exception{
		ArchiveCorrupt( uint32_t corruptionType ) : Utilities::Exception( "Archive is corrupted. \nError: " + std::to_string( corruptionType ) )
		{}
	};
	enum class AccessMode{
		read_only,
		read_write
	};
	typedef std::pair<Utilities::GUID, const Utilities::Memory::Handle> To_Save;
	typedef std::vector<std::pair<Utilities::GUID, const Utilities::Memory::Handle>> To_Save_Vector;
	class IResourceArchive{
	public:
		DECLSPEC_RA static std::unique_ptr<IResourceArchive> create_binary_archive( std::string_view path, AccessMode mode );

		virtual ~IResourceArchive()
		{};

		virtual const size_t			num_resources()const noexcept = 0;
		virtual const bool				exists( const Utilities::GUID ID )const noexcept = 0;

		// Looks up the size of the resource with the given ID.
		//
		//  \exception ResourceNotFound
		virtual const size_t			get_size( const Utilities::GUID ID )const = 0;
			
		// Will get the name of the resource specified
		//
		//  \exception ResourceNotFound
		virtual const std::string		get_name( const Utilities::GUID ID )const = 0;

		virtual const Utilities::GUID	get_type( const Utilities::GUID ID )const = 0;
		virtual void					create( const Utilities::GUID ID ) = 0;

		virtual void save_resource_info() = 0;
		virtual void save_resource_info_data( const To_Save& to_save, Utilities::Memory::ChunkyAllocator& allocator ) = 0;
		virtual void save_resource_info_data( const To_Save_Vector& to_save_vector, Utilities::Memory::ChunkyAllocator& allocator ) = 0;

		// Write a Memory_Block to a resource
		//
		// Memory is only written to RAM. To write to file, see save.
		//  \exception ArchiveReadOnlyMode
		//virtual void					write( const Utilities::GUID ID, const Utilities::Allocators::MemoryBlock data ) = 0;
		virtual void					set_name( const Utilities::GUID ID, std::string_view name ) = 0;
		virtual void					set_type( const Utilities::GUID ID, const Utilities::GUID type ) = 0;
		// Read in the data for the resource
		//
		// Will allocate memory using the allocator specified at creation.
		//  \exception ResourceNotFound
		virtual const Utilities::Memory::Handle read( const Utilities::GUID ID, Utilities::Memory::ChunkyAllocator& allocator ) = 0;

	protected:
		IResourceArchive()
		{};

	};

}
#endif