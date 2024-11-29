#include "decompiler.h"
#include <iostream>
#include <fstream>
#include "json_emitter.h"
#include "reverse_compiler.h"
#include "story_impl.h"

namespace ink::decompiler
{
void reverse(const char* filenameIn, const char* filenameOut)
{
	std::ifstream in(filenameIn, std::ios::binary);
	std::ofstream out(filenameOut);
	reverse(in, out);
}

void reverse(std::istream& in, std::ostream& out)
{
	using namespace internal;
	// read everything to a byte array
	in.seekg(0, std::ios::end);
	size_t size = in.tellg();
	in.seekg(0, std::ios::beg);
	std::vector<unsigned char> buffer(size);
	in.read(reinterpret_cast<char*>(buffer.data()), size);

	ink::runtime::internal::story_impl story(buffer.data(), size, false);
	json_emitter                       emitter(out);
	reverse_compiler                   compiler(story, emitter);
	compiler.decompile();
}
} // namespace ink::decompiler