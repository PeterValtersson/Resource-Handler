#pragma once
#include "ResourceHandler_Read.h"
namespace Resources
{
	class ResourceHandler_Write : public ResourceHandler_Read
	{
	public:
		virtual void register_resource( Utilities::GUID ID );
		virtual void write_data( Utilities::GUID ID, const void* const data, const size_t size );
		virtual void set_type( Utilities::GUID ID, Utilities::GUID type );
		virtual void set_name( Utilities::GUID ID, std::string_view name );

	private:
		struct write_data_info{
			Utilities::GUID ID;
			std::promise<Utilities::Memory::Handle> promise;
		};
		Utilities::CircularFiFo<write_data_info>	write_data_queue;
		void										write_datas()noexcept;

		struct set_type_info{
			Utilities::GUID ID;
			Utilities::GUID type;
		};
		Utilities::CircularFiFo<set_type_info>		set_type_queue;
		void										set_types()noexcept;

	};
}


