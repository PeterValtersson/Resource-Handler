#ifndef _BINARY_RESOURCE_ARCHIVE_H_
#define _BINARY_RESOURCE_ARCHIVE_H_
#include <IResourceArchive.h>
#include <fstream>
#include <Sofa.h>
#include <map>
#include <ChunkyAllocator.h>

namespace ResourceArchive
{
	class BinaryResourceArchive : public IResourceArchive {
	public:
		BinaryResourceArchive( const std::string& archivePath, ArchiveMode mode );
		~BinaryResourceArchive();

		virtual void				save()override;
		virtual bool				exists( Utilities::GUID ID )const noexcept override;
		virtual size_t				getSize( Utilities::GUID ID )const override;
		virtual std::string			getName( Utilities::GUID ID )const override;
		virtual void				write( Utilities::GUID ID, const void* data, size_t size )override;
		virtual void				setName( Utilities::GUID ID, const std::string& name )override;
		virtual void				read( Utilities::GUID ID, const std::function<void( Utilities::Allocators::MemoryBlock )>& callback )override;
	
		//virtual void				write( Utilities::GUID ID, std::string_view name, Memory_Block memory )override;
	private:
		struct LoadedEntryInfo {
			Utilities::GUID ID;
			size_t size;
			std::string name;
			bool saved;
			Utilities::Allocators::Handle memoryHandle;
		};

		struct Header {
			uint32_t version = 000001;
			uint64_t tailStart;
			uint64_t unusedSpace;
		} header;

		struct Entries : public Utilities::Sofa::Array::Sofa<Utilities::GUID, Utilities::GUID::Hasher,
			char[128],	 // Name
			uint64_t, // Data start
			uint64_t	 // Data size
		> {
			static const uint8_t ID = 0;
			static const uint8_t Name = 1;
			static const uint8_t DataStart = 2;
			static const uint8_t DataSize = 3;
		} entries;

		ArchiveMode mode;


		//ArchiveEntry		loadEntry( Utilities::GUID ID );
	//	void				writeEntry( Utilities::GUID ID, Memory_Block memory, std::string_view name );
	private:
		

		void readHeader();
		void readTail();

		void writeHeader();
		void writeTail();

		std::vector<LoadedEntryInfo> loadedEntries;
		std::map<Utilities::GUID, size_t, Utilities::GUID::Compare> guidToEntryIndex;

		std::fstream stream;
		std::string archivePath;




		static const uint32_t lastestVersion = 000001;
		Utilities::Allocators::ChunkyAllocator allocator;
	};
}
#endif