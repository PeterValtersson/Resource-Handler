#pragma once
#include <functional>
#include <Utilities/Memory/ChunkyAllocator.h>
typedef void(__cdecl* parse_callback_signature)();
using parse_callback = std::function<void()>;
