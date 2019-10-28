#include "ResourceHandler_Write.h"
#include <Utilities/Profiler/Profiler.h>

void Resources::ResourceHandler_Write::register_resource( Utilities::GUID ID )
{
	PROFILE;
	if (!archive->exists( ID ))
		archive->create( ID );

	ResourceHandler_Read::register_resource( ID );
}

void Resources::ResourceHandler_Write::write_data( Utilities::GUID ID, const char* const data, size_t size )
{
}

void Resources::ResourceHandler_Write::set_type( Utilities::GUID ID )
{
}

void Resources::ResourceHandler_Write::set_name( Utilities::GUID ID, std::string_view name )
{
}
