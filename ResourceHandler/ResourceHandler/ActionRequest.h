#pragma once
#include <Utilities/GUID.h>
#include <future>

template<typename T>
struct Action_Request {
	struct Response {
		char data[64];

		Response() {}
		template<typename T>
		Response( const T other )
		{
			assert( sizeof( T ) <= 64 );
			memcpy( data, &other, sizeof( T ) );
		}
		template<typename T>
		T& operator=( const T other )
		{
			assert( sizeof( T ) <= 64 );
			memcpy( data, &other, sizeof( T ) );
			return *( T* )data;
		}
		template<typename T>
		T& get()
		{
			return *( T* )data;
		}
	};
	T type;
	Utilities::GUID ID;
	std::promise<Response> promise;
};