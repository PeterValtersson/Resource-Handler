#ifndef _RESOURCE_ARCHIVE_H_
#define _RESOURCE_ARCHIVE_H_
#include <GUID.h>
#include <ErrorHandling.h>
#include <stdint.h>
#include <DLL_Export.h>
#include <ChunkyAllocator.h>
#include <memory>
#include <functional>

namespace Resources
{
	struct ArchivePathNotFound : public Utilities::Exception {
		ArchivePathNotFound( const std::string& path ) : Utilities::Exception( "Archive path could not be found. Path: " + path ) { }
	};

	struct ArchiveResourceNotFound : public Utilities::Exception {
		ArchiveResourceNotFound( Utilities::GUID ID ) : Utilities::Exception( "Could not find archive resource. GUID: " + std::to_string( ID ) ) { }
	};

	struct ArchiveNotInDeveloperMode : public Utilities::Exception {
		ArchiveNotInDeveloperMode( Utilities::GUID ID ) : Utilities::Exception( "Tried to write to archive while not in developer mode.\n GUID: " + std::to_string( ID ) ) { }
		ArchiveNotInDeveloperMode() : Utilities::Exception( "Tried to write to archive while not in developer mode." ) { }
	};

	struct ArchivePathNotAccessible : public Utilities::Exception {
		ArchivePathNotAccessible( const std::string& path ) : Utilities::Exception( "Could not access archive resource. GUID: " + path ) { }
	};

	struct ArchiveCorrupt : public Utilities::Exception {
		ArchiveCorrupt( const std::string& path, uint32_t corruptionType ) : Utilities::Exception( "Archive is corrupted. \nError: " + std::to_string( corruptionType ) ) { }
	};
	enum class AccessMode {
		read_only,
		read_write
	};
	class IResourceArchive {
	public:
		virtual ~IResourceArchive() { };

		// Checks whether or not the given file exists
		//
		virtual bool				exists( const Utilities::GUID ID )const noexcept = 0;

		// Looks up the size of the resource with the given ID.
		//
		//  \exception ArchiveResourceNotFound
		virtual size_t				get_size( const Utilities::GUID ID )const = 0;

		// Will get the name of the resource specified
		//
		//  \exception ArchiveResourceNotFound
		virtual std::string			get_name( const Utilities::GUID ID )const = 0;


		// Save all changes to the resource archive file
		//
		// \warning On shutdown, any unsaved data will be lost
		//  \exception ArchiveNotInDeveloperMode
		virtual void				save( const std::vector<std::pair<Utilities::GUID, Utilities::Allocators::MemoryBlock>>& data_to_save ) = 0;

		// Write a Memory_Block to a resource
		//
		// Memory is only written to RAM. To write to file, see save.
		//  \exception ArchiveNotInDeveloperMode
		//virtual void				write( const Utilities::GUID ID, const Utilities::Allocators::MemoryBlock data ) = 0;
		virtual void				set_name( const Utilities::GUID ID, const std::string& name ) = 0;
		virtual void				set_type( const Utilities::GUID ID, const Utilities::GUID type ) = 0;
		// Read in the data for the resource
		//
		// Will allocate memory using the allocator specified at creation.
		//  \exception ArchiveResourceNotFound
		virtual const Utilities::Allocators::Handle read( const Utilities::GUID ID, Utilities::Allocators::ChunkyAllocator& allocator ) = 0;

	protected:
		IResourceArchive() { };

	};

	DECLSPEC_RA std::unique_ptr<IResourceArchive> createResourceArchive( const std::string& path, AccessMode mode );
}
#endif