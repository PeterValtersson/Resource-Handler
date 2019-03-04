#ifndef _RESOURCE_ARCHIVE_H_
#define _RESOURCE_ARCHIVE_H_
#include <GUID.h>
#include <ErrorHandling.h>
#include <stdint.h>
#include <DLL_Export.h>
#include <MemoryBlock.h>
#include <memory>
#include <functional>

namespace ResourceArchive
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

	enum class ArchiveMode {
		development,
		runtime
	};

	class IResourceArchive {
	public:
		virtual ~IResourceArchive();

		// Save all changes to the resource archive file
		//
		// \warning On shutdown, any unsaved data will be lost
		//  \exception ArchiveNotInDeveloperMode
		virtual void				save() = 0;

		// Checks whether or not the given file exists
		//
		virtual bool				exists( Utilities::GUID ID )const noexcept = 0;

		// Looks up the size of the resource with the given ID.
		//
		//  \exception ArchiveResourceNotFound
		virtual size_t				getSize( Utilities::GUID ID )const = 0;

		// Will get the name of the resource specified
		//
		//  \exception ArchiveResourceNotFound
		virtual std::string			getName( Utilities::GUID ID )const = 0;

		// Write a Memory_Block to a resource
		//
		// Memory is only written to RAM. To write to file, see save.
		//  \exception ArchiveNotInDeveloperMode
		virtual void				write( Utilities::GUID ID, const void* data, size_t size ) = 0;
		virtual void				setName( Utilities::GUID ID, const std::string& name ) = 0;

		// Read in the data for the resource
		//
		// Will allocate memory using the allocator specified at creation.
		//  \exception ArchiveResourceNotFound
		virtual void				read( Utilities::GUID ID, const std::function<void( Utilities::Allocators::MemoryBlock )>& callback ) = 0;
		
		// Read in the data for the resource
		//
		// Will allocate memory using the allocator specified at creation.
		// Is only available in runtime.
		//  \exception ArchiveResourceNotFound
		/*virtual Utilities::Allocators::MemoryBlock	readAndKeep( Utilities::GUID ID, const std::function<void( Utilities::Allocators::MemoryBlock )>& callback ) = 0;
		virtual void				returnKeptRead( Utilities::GUID ID ) = 0;*/
		/*


		// Read in the data for the resource
		//
		// Will allocate memory using the allocator specified at creation.
		//  \exception ArchiveResourceNotFound
		virtual ArchiveEntry		read( Utilities::GUID ID ) = 0;







		// Write a Memory_Block and change name
		//
		// Memory is only written to RAM. To write to file, see save.
		//  \exception ArchiveNotInDeveloperMode
		virtual void				write( Utilities::GUID ID, std::string_view name, Memory_Block memory ) = 0;*/


	protected:
		IResourceArchive();

	};

	DECLSPEC_RA std::unique_ptr<IResourceArchive> createResourceArchive( const std::string& path, ArchiveMode mode );
}
#endif