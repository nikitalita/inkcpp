
#include "binstr_modifier.h"

#include "instruction_reader.h"

#include <fstream>
#include <version.h>

namespace ink
{
namespace decompiler
{

#ifdef INK_ENABLE_STL
	unsigned char* read_file_into_memory(const char* filename, size_t* read)
	{
		using namespace std;

		ifstream ifs(filename, ios::binary | ios::ate);

		if (! ifs.is_open()) {
			throw ink_exception("Failed to open file: " + std::string(filename));
		}

		ifstream::pos_type pos    = ifs.tellg();
		size_t             length = ( size_t ) pos;
		unsigned char*     data   = new unsigned char[length];
		ifs.seekg(0, ios::beg);
		ifs.read(( char* ) data, length);
		ifs.close();

		*read = ( size_t ) length;
		return data;
	}

	binstr_modifier::binstr_modifier(const char* filename)
	    : _file(nullptr)
	    , _length(0)
	    , _string_table(nullptr)
	    , _instruction_data(nullptr)
	    , _managed(true)
	{
		// Load file into memory
		_file = read_file_into_memory(filename, &_length);

		// Find all the right data sections
		setup_pointers();
	}
#endif

	binstr_modifier::binstr_modifier(unsigned char* binary, size_t len, bool manage /*= true*/)
	    : _file(binary)
	    , _length(len)
	    , _managed(manage)
	{
		// Setup data section pointers
		setup_pointers();
	}

	binstr_modifier::~binstr_modifier()
	{
		// delete file memory if we're responsible for it
		if (_file != nullptr && _managed)
			delete[] _file;

		// clear pointers
		_file             = nullptr;
		_instruction_data = nullptr;
		_string_table     = nullptr;

		_block = nullptr;
	}

	std::map<uint32_t, std::string> binstr_modifier::get_string_table() const
	{
		std::map<uint32_t, std::string> strings;
		const char*                     ptr = _string_table;
		if (*ptr == 0) // SPECIAL: No strings
			return strings;
		while (true) {
			auto offset     = ( uint32_t ) (ptr - _string_table);
			strings[offset] = ptr;
			// Read until null terminator
			while (*ptr != 0)
				ptr++;

			// Check next character
			ptr++;

			// Second null. Strings are done.
			if (*ptr == 0) {
				ptr++;
				break;
			}
		}
		return strings;
	}

	size_t binstr_modifier::modify_string_table(
	    const std::map<std::string, std::string>& orig_to_replace, std::ostream& buffer
	) const
	{
		auto startPos  = buffer.tellp();
		auto old_table = get_string_table();
		std::map<uint32_t, std::pair<std::string, std::string>> to_replace;
		for (auto& [offset, oldstr] : old_table) {
			std::pair<std::string, std::string> new_pair;
			new_pair.first = oldstr;
			if (auto it = orig_to_replace.find(oldstr); it != orig_to_replace.end()) {
				new_pair.second = it->second;
			} else {
				new_pair.second = oldstr;
			}
			to_replace[offset] = new_pair;
		}
		std::vector<uint8_t>         new_table;
		std::map<uint32_t, uint32_t> offset_map;
		for (auto& [offset, pair] : to_replace) {
			auto& [oldstr, newstr] = pair;
			offset_map[offset]     = new_table.size();
			for (auto c : newstr) {
				new_table.push_back(c);
			}
			new_table.push_back(0);
		}
		new_table.push_back(0);

		// copy the header
		buffer.write(reinterpret_cast<const char*>(_file), ink::internal::header::Size);
		// auto afterHeader = buffer.tellp();

		// copy the string table
		buffer.write(reinterpret_cast<const char*>(new_table.data()), new_table.size());
		// auto afterTable = buffer.tellp();

		// copy the rest of the data between the end of the string table and the beginning of the
		// instruction data
		auto restStart = ( unsigned char* ) _list_meta;
		if (restStart == nullptr) {
			// _list_meta is undefined, we have to manually push back a null_flag and _num_containers and
			// set start to _container_list
			auto nul_flag = null_flag;
			buffer.write(reinterpret_cast<const char*>(&nul_flag), sizeof(null_flag));
			auto num_containers = _num_containers;
			buffer.write(reinterpret_cast<const char*>(&num_containers), sizeof(uint32_t));
			restStart = ( unsigned char* ) _container_list;
		}
		auto instStart = ( unsigned char* ) _instruction_data;
		buffer.write(reinterpret_cast<const char*>(restStart), instStart - restStart);
		// auto beforeInstructions = buffer.tellp();

		instruction_reader reader(
		    _instruction_data, end(), ( const char* ) new_table.data(), _header.endien
		);
		while (!reader.at_end()) {
			auto inst = reader.read_instruction();
			switch (inst.command) {
				case Command::TAG:
				case Command::STR: {
					auto offset = inst.param.uint_param;
					if (auto it = offset_map.find(offset); it != offset_map.end()) {
						inst.param.uint_param = it->second;
					} else {
						// this shouldn't happen, throw
						throw ink_exception("offset not found in offset_map");
					}
				} break;
				default: break;
			}
			instruction_reader::write_instruction(inst, buffer);
		}
		// auto afterInstructions = buffer.tellp();
		// return the size of the new file
		return buffer.tellp() - startPos;
	}

	void binstr_modifier::setup_pointers()
	{
		using header = ink::internal::header;
		_header      = header::parse_header(reinterpret_cast<char*>(_file));

		// String table is after the header
		_string_table = ( char* ) _file + header::Size;

		// Pass over strings
		const char* ptr = _string_table;
		if (*ptr == 0) // SPECIAL: No strings
		{
			ptr++;
		} else
			while (true) {
				// Read until null terminator
				while (*ptr != 0)
					ptr++;

				// Check next character
				ptr++;

				// Second null. Strings are done.
				if (*ptr == 0) {
					ptr++;
					break;
				}
			}
		_string_table_size = ( size_t ) (ptr - _string_table);

		// check if lists are defined
		_list_meta = ptr;
		if (list_flag flag = _header.read_list_flag(ptr); flag != null_flag) {
			// skip list definitions
			auto list_id = flag.list_id;
			while (*ptr != 0) {
				++ptr;
			}
			++ptr; // skip list name
			do {
				if (flag.list_id != list_id) {
					list_id = flag.list_id;
					while (*ptr != 0) {
						++ptr;
					}
					++ptr; // skip list name
				}
				while (*ptr != 0) {
					++ptr;
				}
				++ptr; // skip flag name
			} while ((flag = _header.read_list_flag(ptr)) != null_flag);

			_lists = reinterpret_cast<const list_flag*>(ptr);
			// skip predefined lists
			while (_header.read_list_flag(ptr) != null_flag) {
				while (_header.read_list_flag(ptr) != null_flag)
					;
			}
		} else {
			_list_meta = nullptr;
			_lists     = nullptr;
		}
		inkAssert(
		    _header.ink_bin_version_number == ink::InkBinVersion,
		    "invalid InkBinVerison! currently: %i you used %i", ink::InkBinVersion,
		    _header.ink_bin_version_number
		);
		inkAssert(
		    _header.endien == header::endian_types::same, "different endien support not yet implemented"
		);


		_num_containers = *( uint32_t* ) (ptr);
		ptr += sizeof(uint32_t);

		// Pass over the container data
		_container_list_size = 0;
		_container_list      = ( uint32_t* ) (ptr);
		while (true) {
			uint32_t val = *( uint32_t* ) ptr;
			if (val == ~0) {
				ptr += sizeof(uint32_t);
				break;
			} else {
				ptr += sizeof(uint32_t) * 2;
				_container_list_size++;
			}
		}

		// Next is the container hash map
		_container_hash_start = ( hash_t* ) (ptr);
		while (true) {
			uint32_t val = *( uint32_t* ) ptr;
			if (val == ~0) {
				_container_hash_end = ( hash_t* ) (ptr);
				ptr += sizeof(uint32_t);
				break;
			}

			ptr += sizeof(uint32_t) * 2;
		}

		// After strings comes instruction data
		_instruction_data = ( ip_t ) ptr;

		// Debugging info
		/*{
		  const uint32_t* iter = nullptr;
		  container_t index; ip_t offset;
		  while (this->iterate_containers(iter, index, offset))
		  {
		    std::clog << "Container #" << index << ": " << (int)offset << std::endl;
		  }
		}*/
	}
} // namespace decompiler
} // namespace ink