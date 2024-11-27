#include "binary_reader.h"
#include <algorithm>

namespace ink::compiler::internal
{
binary_reader::binary_reader(std::istream& input)
    : _input(input)
    , _swap_endian(false)
{
	uint16_t byteorder_marker = read_uint16();
	_swap_endian              = (byteorder_marker != 0x0001);
}

uint8_t binary_reader::read_uint8()
{
	uint8_t value = -1;
	_input.read(reinterpret_cast<char*>(&value), sizeof(value));
	return value;
}

uint16_t binary_reader::read_uint16()
{
	uint16_t value = -1;
	_input.read(reinterpret_cast<char*>(&value), sizeof(value));
	if (_swap_endian) {
		value = (value >> 8) | (value << 8);
	}
	return value;
}

int binary_reader::read_int()
{
	int value = -1;
	_input.read(reinterpret_cast<char*>(&value), sizeof(value));
	if (_swap_endian) {
		value = __builtin_bswap32(value);
	}
	return value;
}

std::string binary_reader::read_string()
{
	std::string result;
	char        ch;
	while (_input.get(ch) && ch != '\0') {
		result += ch;
	}
	return result;
}

std::vector<uint8_t> binary_reader::read_bytes(size_t count)
{
	std::vector<uint8_t> buffer(count);
	_input.read(reinterpret_cast<char*>(buffer.data()), count);
	return buffer;
}
} // namespace ink::compiler::internal