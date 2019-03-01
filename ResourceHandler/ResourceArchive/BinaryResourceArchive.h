#ifndef _BINARY_RESOURCE_ARCHIVE_H_
#define _BINARY_RESOURCE_ARCHIVE_H_
#include <IResourceArchive.h>
#include <fstream>
#include <Sofa.h>
#include <map>

namespace ResourceArchive
{
	class BinaryResourceArchive: public IResourceArchive {
	public:
		BinaryResourceArchive( const std::string& archivePath, ArchiveMode mode);
		~BinaryResourceArchive();

		virtual bool				exists( Utilities::GUID ID )const noexcept;
		virtual size_t				size( Utilities::GUID ID )const;
		virtual std::string_view	name( Utilities::GUID ID )const;
		virtual const ArchiveInfo&	read( Utilities::GUID ID );
		virtual void				write( Utilities::GUID ID, Memory_Block& memory );

	private:


		struct Archive  {
			struct Header {
				uint32_t version = 000001;
				uint64_t tailStart;
				uint64_t tailSize;
			} header;

			struct Entries : public Utilities::Sofa::Array::Sofa<Utilities::GUID, Utilities::GUID::Hasher,
				char[128],	 // Name
				std::size_t, // Data start
				std::size_t	 // Data size
			> {
				static const uint8_t ID = 0;
				static const uint8_t Name = 1;
				static const uint8_t DataStart = 2;
				static const uint8_t DataSize = 3;
			} entries;

			
			const ArchiveInfo&	loadEntry( Utilities::GUID ID , size_t index);

			
		

			Archive( const std::string& archivePath, ArchiveMode mode );
			~Archive();

		private:
			void readHeader();
			void readTail();

			void writeHeader();
		

			std::map<Utilities::GUID, ArchiveInfo, Utilities::GUID::Compare> loadedEntries;
			std::fstream stream;
		} _archive;


		static const uint32_t lastestVersion = 000001;
		ArchiveMode mode;
	};
}
#endif