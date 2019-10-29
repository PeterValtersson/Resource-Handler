#pragma once
#include "ResourceHandler_Read.h"
namespace Resources
{
	class ResourceHandler_Write : public ResourceHandler_Read
	{
	public:
		virtual void register_resource( Utilities::GUID ID );
		virtual void write_data( Utilities::GUID ID, const char* const data, size_t size );
		virtual void set_type( Utilities::GUID ID );
		virtual void set_name( Utilities::GUID ID, std::string_view name );

	private:
		struct write_data_info{
			Utilities::GUID ID;
			std::promise<Utilities::Memory::Handle> promise;
		};
		Utilities::CircularFiFo<write_data_info>	write_data_queue;
		void										write_datas();


	};
}


