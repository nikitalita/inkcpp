#include "decompiler.h"

#include "binstr_modifier.h"

#include <iostream>
#include <fstream>
#include "json_emitter.h"
#include "reverse_compiler.h"
#include "story_impl.h"

namespace ink::decompiler
{
void reverse(const char* filenameIn, const char* filenameOut)
{
	if (!filenameIn || !filenameOut) {
		throw std::invalid_argument("filenameIn and filenameOut must not be nullptr");
	}
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

void write_string_table_json(const char* binfileIn, const char* jsonfileOut)
{
	if (!binfileIn || !jsonfileOut) {
		throw std::invalid_argument("binfileIn and jsonfileOut must not be blank!");
	}
	std::ifstream in(binfileIn, std::ios::binary);
	if (!in) {
		throw std::invalid_argument(std::string(binfileIn) + " does not exist.");
	}
	std::ofstream out(jsonfileOut, std::ios::out | std::ios::trunc);
	if (!out) {
		throw std::invalid_argument(std::string(jsonfileOut) + " cannot be created.");
	}
	write_string_table_json(in, out);
}

void write_string_table_json(std::istream& binIn, std::ostream& jsonOut)
{
	using namespace internal;
	// read everything to a byte array
	binIn.seekg(0, std::ios::end);
	size_t size = binIn.tellg();
	binIn.seekg(0, std::ios::beg);
	std::vector<unsigned char> buffer(size);
	binIn.read(reinterpret_cast<char*>(buffer.data()), size);

	binstr_modifier modifier(buffer.data(), size, false);
	auto            table = modifier.get_string_table();
	nlohmann::json  json_table;
	// just "string": "string" so that people can easily edit it
	for (auto& [offset, str] : table) {
		json_table[str] = str;
	}
	jsonOut << json_table.dump(4, ' ', false, nlohmann::detail::error_handler_t::replace)
	        << std::endl;
}

void modify_string_table(const char* binfileIn, const char* jsonfileIn, const char* binfileOut)
{
	if (!binfileIn || !jsonfileIn || !binfileOut) {
		throw std::invalid_argument("binfileIn, jsonfileIn and binfileOut must not be nullptr");
	}
	std::ifstream binIn(binfileIn, std::ios::binary);
	if (!binIn) {
		throw std::invalid_argument(std::string(binfileIn) + " does not exist.");
	}
	std::ifstream jsonIn(jsonfileIn);
	if (!jsonIn) {
		throw std::invalid_argument(std::string(jsonfileIn) + " does not exist.");
	}
	std::ofstream binOut(binfileOut, std::ios::binary | std::ios::out | std::ios::trunc);
	if (!binOut) {
		throw std::invalid_argument(std::string(binfileOut) + " cannot be created.");
	}
	modify_string_table(binIn, jsonIn, binOut);
}

void modify_string_table(std::istream& binIn, std::istream& jsonIn, std::ostream& binOut)
{
	using namespace internal;
	// read everything to a byte array
	binIn.seekg(0, std::ios::end);
	size_t size = binIn.tellg();
	binIn.seekg(0, std::ios::beg);
	std::vector<unsigned char> buffer(size);
	binIn.read(reinterpret_cast<char*>(buffer.data()), size);

	binstr_modifier modifier(buffer.data(), size, false);
	auto            table = modifier.get_string_table();
	nlohmann::json  json_table;
	jsonIn >> json_table;
	std::map<std::string, std::string> orig_to_replace;
	for (auto& [key, value] : json_table.items()) {
		orig_to_replace[key] = value.get<std::string>();
	}
	std::vector<unsigned char> new_buffer;
	modifier.modify_string_table(orig_to_replace, binOut);
}
} // namespace ink::decompiler