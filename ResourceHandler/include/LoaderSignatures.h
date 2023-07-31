#pragma once
#include <functional>
#include <Utilities/Memory/Allocator.h>
#include <istream>

typedef void(__cdecl* parse_callback_signature)(Utilities::Memory::Allocator*, std::istream* stream);
using parse_callback = std::function<void(Utilities::Memory::Allocator*, std::istream* stream)>;
