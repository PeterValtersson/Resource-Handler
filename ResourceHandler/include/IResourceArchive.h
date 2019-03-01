#ifndef _RESOURCE_ARCHIVE_H_
#define _RESOURCE_ARCHIVE_H_
#include <GUID.h>
#include <string_view>
#include <ErrorHandling.h>
#include <stdint.h>
#include <memory>
#include <DLL_Export.h>

namespace ResourceArchive
{
	struct ArchivePathNotFound : public Utilities::Exception {
		ArchivePathNotFound( const std::string& path ) : Utilities::Exception( "Archive path could not be found. Path: " + path ) { }
	};

	struct ArchiveResourceNotFound : public Utilities::Exception {
		ArchiveResourceNotFound( Utilities::GUID ID ) : Utilities::Exception( "Could not find archive resource. ID: " + std::to_string( ID ) ) { }
	};

	struct ArchiveNotInDeveloperMode : public Utilities::Exception {
		ArchiveNotInDeveloperMode( Utilities::GUID ID ) : Utilities::Exception( "Tried to write to archive while not in developer mode.\n GUID: " + std::to_string( ID ) ) { }
	};

	struct ArchivePathNotAccessible : public Utilities::Exception {
		ArchivePathNotAccessible( const std::string& path ) : Utilities::Exception( "Could not access archive resource. ID: " + path ) { }
	};

	struct ArchiveCorrupt : public Utilities::Exception {
		ArchiveCorrupt( const std::string& path, uint32_t corruptionType ) : Utilities::Exception( "Archive is corrupted. \nError: " + std::to_string( corruptionType ) ) { }
	};

	enum class ArchiveMode {
		development,
		runtime
	};
	struct Memory_Block {
		void* data = nullptr;
		size_t size = 0;
	};
	struct ArchiveInfo {
		Utilities::GUID ID;
		std::string name;
		Memory_Block memory;
	};
	class IResourceArchive {
	public:
		virtual ~IResourceArchive();

		virtual bool				exists( Utilities::GUID ID )const noexcept = 0;
		// Looks up the size of the resource with the given ID.
		//
		//  \exception TODO
		virtual size_t				size( Utilities::GUID ID )const = 0;

		// Will get the name of the resource specified
		//
		virtual std::string_view	name( Utilities::GUID ID )const = 0;

		// Read in the data for the resource
		//
		// Will allocate memory using the allocator specified at creation.
		virtual const ArchiveInfo&	read( Utilities::GUID ID ) = 0;

		// Write to the resource
		//
		virtual void				write( Utilities::GUID ID, Memory_Block& memory ) = 0;
	protected:
		IResourceArchive();
	};

	DECLSPEC_RA std::unique_ptr<IResourceArchive> createResourceArchive( const std::string& path, ArchiveMode mode );
}
#endif