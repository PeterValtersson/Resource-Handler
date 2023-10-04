#ifndef _RESOURCE_HANDLER_READ_H_
#define _RESOURCE_HANDLER_READ_H_
#include <IResourceHandler.h>
#include "ResourceHandler_Read_Imp.h"
#include "ActionRequest.h"

namespace ResourceHandler
{


	class ResourceHandler_Read : public IResourceHandler {
	public:
		ResourceHandler_Read( std::shared_ptr<IResourceArchive> archive );
		~ResourceHandler_Read();

		virtual void set_parser_memory_handler(std::shared_ptr<Utilities::Memory::Allocator> memory_handler) override;
		virtual std::shared_ptr<IResourceArchive> get_archive()override;
		virtual void add_parser(const Utilities::GUID type, const std::string& library_path)override;
		virtual void add_parser(const Utilities::GUID type, const parse_callback& parse_callback) override;
	private:
		virtual void			register_resource( const Utilities::GUID ID, const Flags flag = Flags::None)noexcept override;
		virtual Status			get_status( const Utilities::GUID ID )noexcept  override;
		virtual void			set_flag(const Utilities::GUID ID, const Flags flag)noexcept override;
		virtual void			remove_flag(const Utilities::GUID ID, const Flags flag)noexcept override;
		virtual void			inc_refCount( const Utilities::GUID ID )noexcept override;
		virtual void			dec_refCount( const Utilities::GUID ID )noexcept  override;
		virtual RefCount		get_refCount( const Utilities::GUID ID )const noexcept override;
		virtual std::string		get_name( const Utilities::GUID ID )const  override;
		virtual void			use_data( const Utilities::GUID ID, const std::function<void( const Utilities::Memory::ConstMemoryBlock )>& callback )override;


	private:
		void update()noexcept;

	private:
		bool running;
		std::thread thread;

		std::shared_ptr<IResourceArchive> archive;
		Utilities::Concurrent<Utilities::Memory::ChunkyAllocator> allocator;

		ResourceHandler_Read_Imp implementation;

		enum class RequestType {
			Register_Resource,
			Get_Status,
			Inc_RefCount,
			Dec_RefCount,
			Get_RefCount,
			Use_Data
		};
		using Request = Action_Request<RequestType>;
		mutable Utilities::CircularFiFo<Request> action_request_queue;
		std::map<RequestType, std::function<Request::Response( Utilities::GUID )>> action_map;
		void perform_actions();



	};
}
#endif