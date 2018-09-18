#ifndef _RESOURCE_H_
#define _RESOURCE_H_
#include <GUID.h>
#include "DLL_Export.h"
#include <Client.h>
#include <exception>

namespace Resources
{
	enum class Status {
		Loaded = 1 << 0,
		Failed = 1 << 1,
		Invalid = 1 << 2,
		Unloaded = 1 << 3
	};
	struct MemoryBlock {
		void* dataPtr;
		size_t dataSize;
	};
	struct ResourceInfo {
		Utilities::GUID guid;
		Utilities::GUID type;
	};
	struct CouldNotLoad : public std::runtime_error {
		CouldNotLoad(const ResourceInfo& info)
			: std::runtime_error("A resource could not be loaded. GUID: " + std::to_string(info.guid.id) + " Type: " + std::to_string(info.type.id) )
		{}
	};
	class Resource_resourceData {
	public:
		~Resource_resourceData()noexcept
		{

		}
		Resource_resourceData()
		{
		}
		const void* get()const
		{
			return _resourceData.dataPtr;
		}
		const Status status()const
		{
			return _status;
		}
	private:
		
		MemoryBlock _resourceData;
		Status _status;
	};
	

	template<typename DataType>
	class Resource {
	public:
		Resource(const Utilities::GUID guid, const Utilities::GUID type) : _info{guid,type}
		{
			auto client = MP::getLocalClient();
			_resourceFuture = client->sendMessage({ "ResourceHandler", "RegisterResource", info });
		}
		~Resource()noexcept
		{
			
		}
		inline const DataType& get()
		{
			if (_resourceFuture.valid())
			{
				auto futureResult = _resourceFuture.get();
				if (!futureResult)
					throw CouldNotLoad(_info);
				_resourceData = futureResult->get<std::shared_ptr<Resource_resourceData>>();
				if (!_resourceData)
					throw CouldNotLoad(_info);	
			}
			if (_resourceData->status() == Status::Unloaded || _resourceData->status == Status::Invalid)
			{
				auto client = MP::getLocalClient();
				auto loadFuture = client->sendMessage({ "ResourceHandler", "LoadResource", info });
				loadFuture.wait();
				if (_resourceData->status() != Status::Loaded)
					throw CouldNotLoad(_info);
			}
			return *reinterpret_cast<const DataType*>(_resourceData.get());
		}
		inline operator const DataType&() { return get(); }
		inline DataType& operator*() { return get(); }
		inline const DataType* operator->() { return &get(); }
		inline const size_t size() { return _resourceData.dataSize; }

		inline const Utilities::GUID guid()const
		{
			return _guid;
		}
		inline const Utilities::GUID type()const
		{
			return _type;
		}
	private:
		ResourceInfo _info;
		std::shared_ptr<Resource_resourceData> _resourceData;
		MP::MessageReturn _resourceFuture;
	};

}
#endif