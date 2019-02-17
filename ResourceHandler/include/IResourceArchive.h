#ifndef _RESOURCE_ARCHIVE_H_
#define _RESOURCE_ARCHIVE_H_
#include <GUID.h>
#include <string_view>

namespace ResourceArchive
{

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
		virtual std::string			name( Utilities::GUID ID )const = 0;

		// Read in the data for the resource
		//
		// Vill allocate memory using the allocator specified at creation.
		virtual ArchiveInfo			read( Utilities::GUID ID ) = 0;

		// Write to the resource
		//
		virtual void				write( Utilities::GUID ID, Memory_Block& memory ) = 0;

	protected:
		IResourceArchive();
	};
}
#endif