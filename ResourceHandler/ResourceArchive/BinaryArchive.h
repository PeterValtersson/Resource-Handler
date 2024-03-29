#ifndef _BINARY_RESOURCE_ARCHIVE_H_
#define _BINARY_RESOURCE_ARCHIVE_H_
#include <IResourceArchive.h>
#include <fstream>
#include <Utilities/Memory/Sofa.h>
#include <map>
#include <Utilities/Memory/ChunkyAllocator.h>

namespace ResourceHandler
{
	class BinaryArchive : public IResourceArchive {
	public:
		BinaryArchive( std::string_view archivePath, AccessMode mode );
		~BinaryArchive();

		const size_t			num_resources()const noexcept final;
		Utilities::GUID			create_from_name( std::string_view name )final;
		void					create_from_ID( const Utilities::GUID ID )final;
		void					create( const Utilities::GUID ID, std::string_view name )final;
		void					save( const To_Save& to_save, Utilities::Memory::Allocator& allocator )final;
		void					save_multiple( const To_Save_Vector& to_save_vector, Utilities::Memory::Allocator& allocator )final;
		const bool				exists( const Utilities::GUID ID )const noexcept final;
		const size_t			get_size( const Utilities::GUID ID )const final;
		const std::string		get_name( const Utilities::GUID ID )const final;
		const Utilities::GUID	get_type( const Utilities::GUID ID )const final;
		void					set_name( const Utilities::GUID ID, std::string_view name )final;
		void					set_type( const Utilities::GUID ID, const Utilities::GUID type )final;
		virtual std::vector<ResourceMetaInfo> get_resources()const noexcept final;
		virtual std::vector<ResourceMetaInfo> get_resources_by_type(const Utilities::GUID type)const noexcept final;

		const Utilities::Memory::Handle	read( const Utilities::GUID ID, Utilities::Memory::Allocator& allocator )final;
		const Utilities::Memory::Handle read(const Utilities::GUID ID, Utilities::Memory::Allocator& allocator, const parse_callback& parser)final;

	private:
		void open();
		struct Header {
			uint32_t version = 000001;
			uint64_t tailStart;
			uint64_t unusedSpace;
		} header;

		struct Entries : public Utilities::Memory::SofA<Utilities::GUID, Utilities::GUID::Hasher,
			char[128],	 // Name
			Utilities::GUID, // Type
			uint64_t, // Data start
			uint64_t	 // Data size
		> {
			static constexpr uint8_t ID = 0;
			static constexpr uint8_t Name = 1;
			static constexpr uint8_t Type = 2;
			static constexpr uint8_t DataStart = 3;
			static constexpr uint8_t DataSize = 4;
		} entries;

	private:
		void save_resource_info();
		void _save_resource_info_data( const To_Save& to_save, Utilities::Memory::Allocator& allocator );

		void readHeader();
		void readTail();

		void writeHeader();
		void writeTail();

		AccessMode mode;
		std::fstream stream;
		std::string archivePath;

		static const uint32_t lastestVersion = 000001;
	};
}
#endif