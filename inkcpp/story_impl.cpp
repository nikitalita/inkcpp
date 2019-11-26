#include "platform.h"
#include "runner_impl.h"
#include "globals_impl.h"
#include "story_impl.h"

#ifdef INK_ENABLE_STL
#include <iostream>
#endif

namespace ink::runtime
{
	story* story::from_file(const char* filename)
	{
		return new internal::story_impl(filename);
	}

	story* story::from_binary(unsigned char* data, size_t length, bool freeOnDestroy)
	{
		return new internal::story_impl(data, length, freeOnDestroy);
	}
}

namespace ink::runtime::internal
{
#ifdef INK_ENABLE_STL
	unsigned char* read_file_into_memory(const char* filename, size_t* read)
	{
		using namespace std;

		ifstream ifs(filename, ios::binary | ios::ate);
		ifstream::pos_type pos = ifs.tellg();
		size_t length = (size_t)pos;
		unsigned char* data = new unsigned char[length];
		ifs.seekg(0, ios::beg);
		ifs.read((char*)data, length);
		ifs.close();

		*read = (size_t)length;
		return data;
	}

	story_impl::story_impl(const char* filename)
		: _file(nullptr)
		, _string_table(nullptr)
		, _instruction_data(nullptr)
		, _length(0)
		, _managed(true)
	{
		// Load file into memory
		_file = read_file_into_memory(filename, &_length);

		// Find all the right data sections
		setup_pointers();

		// create story block
		_block = new internal::ref_block();
		_block->references = 1;
	}
#endif

	story_impl::story_impl(unsigned char* binary, size_t len, bool manage /*= true*/)
		: _file(binary), _length(len), _managed(manage)
	{
		// Setup data section pointers
		setup_pointers();
	}

	story_impl::~story_impl()
	{
		// delete file memory if we're responsible for it
		if (_file != nullptr && _managed)
			delete[] _file;

		// clear pointers
		_file = nullptr;
		_instruction_data = nullptr;
		_string_table = nullptr;

		// clear out our reference block
		_block->valid = false;
		internal::ref_block::remove_reference(_block);
		_block = nullptr;
	}

	const char* story_impl::string(uint32_t index) const
	{
		return _string_table + index;
	}

	bool story_impl::iterate_containers(const uint32_t*& iterator, container_t& index, ip_t& offset, bool reverse) const
	{
		if (iterator == nullptr)
		{
			// Empty check
			if (_container_list_size == 0)
			{
				return false;
			}

			// Start
			iterator = reverse
				? _container_list + (_container_list_size - 1) * 2
				: _container_list;
		}
		else
		{
			// Range check
			assert(iterator >= _container_list && iterator <= _container_list + _container_list_size * 2);

			// Advance
			iterator += reverse ? -2 : 2;

			// End?
			if (iterator >= _container_list + _container_list_size * 2 || iterator < _container_list)
			{
				iterator = nullptr;
				index = 0;
				offset = nullptr;
				return false;
			}
		}

		// Get metadata
		index = *(iterator + 1);
		offset = *iterator + instructions();
		return true;
	}

	bool story_impl::get_container_id(ip_t offset, container_t& container_id) const
	{
		const uint32_t* iter = nullptr;
		ip_t iter_offset = nullptr;
		while (iterate_containers(iter, container_id, iter_offset))
		{
			if (iter_offset == offset)
				return true;
		}

		return false;
	}

	globals story_impl::new_globals()
	{
		return globals(new globals_impl(this), _block);
	}

	runner story_impl::new_runner(globals store)
	{
		if (store == nullptr)
			store = new_globals();
		return runner(new runner_impl(this, store), _block);
	}

	void story_impl::setup_pointers()
	{
		// String table is after the version information
		_string_table = (char*)_file + sizeof(int);

		// Pass over strings
		const char* ptr = _string_table;
		while (true)
		{
			// Read until null terminator
			while (*ptr != 0)
				ptr++;

			// Check next character
			ptr++;

			// Second null. Strings are done.
			if (*ptr == 0)
			{
				ptr++;
				break;
			}
		}

		_num_containers = *(uint32_t*)(ptr);
		ptr += sizeof(uint32_t);

		// Pass over the container data
		_container_list_size = 0;
		_container_list = (uint32_t*)(ptr);
		while (true)
		{
			uint32_t val = *(uint32_t*)ptr;
			if (val == ~0)
			{
				ptr += sizeof(uint32_t);
				break;
			}
			else
			{
				ptr += sizeof(uint32_t) * 2;
				_container_list_size++;
			}
		}

		// After strings comes instruction data
		_instruction_data = (ip_t)ptr;

		{
			const uint32_t* iter = nullptr;
			container_t index; ip_t offset;
			while (this->iterate_containers(iter, index, offset))
			{
				std::clog << "Container #" << index << ": " << (int)offset << std::endl;
			}
		}
	}
}