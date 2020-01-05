//#ifndef _ZIP_ARCHIVE_H_
//#define _ZIP_ARCHIVE_H_
//#pragma once
//#include "IResourceArchive.h"
//#include <Utilities/Memory/Sofa.h>
//#include "Bit7zArchive.h"
//
//namespace Resources
//{
//	class RZIPArchive : public IResourceArchive {
//	public:
//		RZIPArchive( std::string_view archivePath, AccessMode mode );
//		~RZIPArchive();
//
//		const size_t			num_resources()const noexcept final;
//		Utilities::GUID			create_from_name( std::string_view name )final;
//		void					create_from_ID( const Utilities::GUID ID )final;
//		void					create( const Utilities::GUID ID, std::string_view name )final;
//		void					save( const To_Save& to_save, Utilities::Memory::ChunkyAllocator& allocator )final;
//		void					save_multiple( const To_Save_Vector& to_save_vector, Utilities::Memory::ChunkyAllocator& allocator )final;
//		const bool				exists( const Utilities::GUID ID )const noexcept final;
//		const size_t			get_size( const Utilities::GUID ID )const final;
//		const std::string		get_name( const Utilities::GUID ID )const final;
//		const Utilities::GUID	get_type( const Utilities::GUID ID )const final;
//		void					set_name( const Utilities::GUID ID, std::string_view name )final;
//		void					set_type( const Utilities::GUID ID, const Utilities::GUID type )final;
//
//		const Utilities::Memory::Handle		read( const Utilities::GUID ID, Utilities::Memory::ChunkyAllocator& allocator )final;
//
//	private:
//		AccessMode mode;
//		std::string archive_path;
//		Bit7zArchive::Ptr archive;
//
//		struct Entries : public Utilities::Memory::SofA<Utilities::GUID, Utilities::GUID::Hasher,
//			char[128]		 // Name
//		> {
//			static constexpr uint8_t ID = 0;
//			static constexpr uint8_t Name = 1;
//		} entries;
//
//	private:
//		void save_entries()noexcept;
//	};
//
//}
//#endif
