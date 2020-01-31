#ifndef _RESOURCE_HANDLER_READ_H_
#define _RESOURCE_HANDLER_READ_H_
#include <IResourceHandler.h>
#include <Utilities/CircularFIFO.h>
#include <thread>
#include <Utilities/Memory/Sofa.h>
#include <Utilities/Memory/ChunkyAllocator.h>
#include <future>
#include <Utilities/Concurrent.h>

namespace ResourceHandler
{
	class ResourceHandler_Read : public IResourceHandler {
	public:
		ResourceHandler_Read( std::shared_ptr<IResourceArchive> archive );
		~ResourceHandler_Read();


	protected:
		virtual void			register_resource( const Utilities::GUID ID )noexcept override;
		virtual void			inc_refCount( const Utilities::GUID ID )noexcept  override;
		virtual void			dec_refCount( const Utilities::GUID ID )noexcept  override;
		virtual RefCount		get_refCount( const Utilities::GUID ID )const noexcept override;
		virtual std::string		get_name( const Utilities::GUID ID )const  override;
		virtual void			use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback )noexcept override;

		struct Action_Request {
			struct Response {
				char data[64];

				Response()
				{

				}
				template<typename T>
				Response(const T other)
				{
					assert( sizeof( T ) <= 64 );
					memcpy( data, &other, sizeof( T ) );
				}
				template<typename T>
				T& operator=( const T other )
				{
					assert( sizeof( T ) <= 64 );
					memcpy( data, &other, sizeof( T ) );
					return *(T*)data;
				}
				template<typename T>
				T& get( )
				{
					return *(T*)data;
				}
			};
			enum class Type {
				Register_Resource,
				Inc_RefCount,
				Dec_RefCount,
				Get_RefCount,
				Use_Data
			};
			Type type;
			Utilities::GUID ID;
			std::promise<Response> promise;
		};
		mutable Utilities::CircularFiFo<Action_Request> action_request_queue;
		std::map<Action_Request::Type, std::function< Action_Request::Response( Utilities::GUID )>> action_map;
		void perform_actions();

		Action_Request::Response _register_resource( Utilities::GUID ID );
		Action_Request::Response _inc_refCount( Utilities::GUID ID );
		Action_Request::Response _dec_refCount( Utilities::GUID ID );
		Action_Request::Response _get_refCount( Utilities::GUID ID );
		Action_Request::Response _use_data( Utilities::GUID ID );



		struct Entries : public Utilities::Memory::SofA<Utilities::GUID, Utilities::GUID::Hasher,
			Status, // Status,
			Utilities::Memory::Handle,	 // Raw
			Utilities::Memory::Handle,	 // Parsed / VRAM info(Vertex Count, etc.)
			RefCount
		> {
			static const uint8_t ID = 0;
			static const uint8_t Status = 1;
			static const uint8_t Memory_Raw = 2;
			static const uint8_t Memory_Parsed = 3;
			static const uint8_t RefCount = 4;
		} resources;

		void update()noexcept;

		std::shared_ptr<IResourceArchive> archive;
		Utilities::Concurrent<Utilities::Memory::ChunkyAllocator> allocator;
		std::thread thread;
		std::vector<std::string> log;
		bool running;

		struct Load_Parse_Info {
			Utilities::GUID ID;
			std::promise<Utilities::Memory::Handle> promise;
			std::future<Utilities::Memory::Handle> future;
			bool load;
		};

		class Loader {
		public:
			Loader( std::shared_ptr<IResourceArchive> archive, Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator )
				: archive( archive ), allocator( allocator )
			{}
			void start()noexcept;
			void stop()noexcept;
			void update(
				std::function<void( Utilities::GUID ID, Utilities::Memory::Handle handle, Status status )> do_if_finished,
				std::function<Utilities::optional<Utilities::GUID>()> choose_to_load )noexcept;

			void run()noexcept;
		private:


			bool running;
			Load_Parse_Info to_load;
			std::thread thread;

			std::shared_ptr<IResourceArchive> archive;
			Utilities::Concurrent<Utilities::Memory::ChunkyAllocator>& allocator;
		}loader;
		void resource_loading_finished( Utilities::GUID ID, Utilities::Memory::Handle handle, Status status ); //Messes with std::function/std::bind noexcept;
		Utilities::optional<Utilities::GUID> choose_resource_to_load(); //Messes with std::function/std::bind noexcept;
	};
}
#endif