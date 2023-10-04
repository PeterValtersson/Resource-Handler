#pragma once
#include <functional>
#include <Utilities/Memory/Allocator.h>
#include <istream>

typedef Utilities::Memory::Handle(__cdecl* parse_callback_signature)(Utilities::Memory::Allocator*, std::istream* stream);
using parse_callback = std::function<Utilities::Memory::Handle(Utilities::Memory::Allocator*, std::istream* stream)>;
