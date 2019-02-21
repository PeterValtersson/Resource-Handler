#ifndef _RESOURCE_ARCHIVE_H_
#define _RESOURCE_ARCHIVE_H_
#include <GUID.h>
#include <string_view>
#include <ErrorHandling.h>

namespace ResourceArchive
{
	struct ArchivePathNotFound : public Utilities::Exception {
		ArchivePathNotFound( const std::string& path ) : Utilities::Exception( "Archive path could not be found. Path: " + path ) { }
	};

	struct ArchiveResourceNotFound : public Utilities::Exception {
		ArchiveResourceNotFound( Utilities::GUID ID ) : Utilities::Exception( "Could not find archive resource. ID: " + std::to_string( ID ) ) { }
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
		std::string_view name;
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
		virtual ArchiveInfo			read( Utilities::GUID ID ) = 0;

		// Write to the resource
		//
		virtual void				write( Utilities::GUID ID, Memory_Block& memory ) = 0;

	protected:
		IResourceArchive();
	};
}
#endif