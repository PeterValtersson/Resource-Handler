#ifndef _BINARY_RESOURCE_ARCHIVE_H_
#define _BINARY_RESOURCE_ARCHIVE_H_
#include <IResourceArchive.h>

namespace ResourceArchive
{
	class BinaryResourceArchive: public IResourceArchive {
	public:
		BinaryResourceArchive();
		~BinaryResourceArchive();

		virtual bool				exists( Utilities::GUID ID )const noexcept;
		virtual size_t				size( Utilities::GUID ID )const;
		virtual std::string			name( Utilities::GUID ID )const;
		virtual ArchiveInfo			read( Utilities::GUID ID );
		virtual void				write( Utilities::GUID ID, Memory_Block& memory );
	};
}
#endif