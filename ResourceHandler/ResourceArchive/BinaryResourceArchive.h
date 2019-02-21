#ifndef _BINARY_RESOURCE_ARCHIVE_H_
#define _BINARY_RESOURCE_ARCHIVE_H_
#include <IResourceArchive.h>
#include <fstream>
#include <Sofa.h>

namespace ResourceArchive
{
	class BinaryResourceArchive: public IResourceArchive {
	public:
		BinaryResourceArchive( const std::string& archivePath, ArchiveMode mode);
		~BinaryResourceArchive();

		virtual bool				exists( Utilities::GUID ID )const noexcept;
		virtual size_t				size( Utilities::GUID ID )const;
		virtual std::string_view	name( Utilities::GUID ID )const;
		virtual ArchiveInfo			read( Utilities::GUID ID );
		virtual void				write( Utilities::GUID ID, Memory_Block& memory );

	private:
		
	private:
		std::fstream _archiveStream;
		ArchiveMode _mode;

		

		struct Archive  {
			struct Header : public Utilities::Sofa::Array::Sofa<Utilities::GUID, Utilities::GUID::Hasher,
				char[512],	 // Name
				std::size_t, // Data start
				std::size_t	 // Data size
			> {
				static const uint8_t ID = 0;
				static const uint8_t name = 1;
				static const uint8_t dataStart = 2;
				static const uint8_t dataSize = 3;
			} header;
			ArchiveInfo	loadArchiveResource( std::size_t index );

		private:
			
		} _archive;
	};
}
#endif