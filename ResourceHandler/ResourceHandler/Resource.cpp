#include <Resource.h>
#include <IResourceHandler.h>

Resources::Resource_Base::Resource_Base( Utilities::GUID ID ) : ID( ID ), checkedIn( false )
{
	IResourceHandler::get()->registerResource( ID );
}
