#ifndef _BINARY_RESOURCE_ARCHIVE_H_
#define _BINARY_RESOURCE_ARCHIVE_H_
#include <IResourceArchive.h>
#include <fstream>
#include <Sofa.h>
#include <map>
#include <ChunkyAllocator.h>

namespace Resources {
	class BinaryArchive : public IResourceArchive {
	public:
		BinaryArchive( const std::string& archivePath, AccessMode mode );
		~BinaryArchive();

		virtual void				save( const std::vector<std::pair<Utilities::GUID, Utilities::Allocators::MemoryBlock>>& data_to_save )override;
		virtual bool				exists( const Utilities::GUID ID )const noexcept override;
		virtual size_t				get_size( const Utilities::GUID ID )const override;
		virtual std::string			get_name( const Utilities::GUID ID )const override;
		//virtual void				write( const Utilities::GUID ID, const Utilities::Allocators::MemoryBlock data )override;
		virtual void				set_name( const Utilities::GUID ID, const std::string& name )override;
		virtual void				set_type( const Utilities::GUID ID, const Utilities::GUID type )override;

		virtual const Utilities::Allocators::Handle		read( const Utilities::GUID ID, Utilities::Allocators::ChunkyAllocator& allocator )override;

		//virtual void				write( Utilities::GUID ID, std::string_view name, Memory_Block memory )override;
	private:
		struct Header {
			uint32_t version = 000001;
			uint64_t tailStart;
			uint64_t unusedSpace;
		} header;

		struct Entries : public Utilities::Sofa::Array::Sofa<Utilities::GUID, Utilities::GUID::Hasher,
			char[128],	 // Name
			Utilities::GUID, // Type
			uint64_t, // Data start
			uint64_t	 // Data size
		> {
			static const uint8_t ID = 0;
			static const uint8_t Name = 1;
			static const uint8_t Type = 2;
			static const uint8_t DataStart = 3;
			static const uint8_t DataSize = 4;
		} entries;

	private:


		void readHeader();
		void readTail();

		void writeHeader();
		void writeTail();

		std::fstream stream;
		std::string archivePath;

		static const uint32_t lastestVersion = 000001;
	};
}
#endif