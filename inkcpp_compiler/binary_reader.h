#pragma once

#include <istream>
#include <vector>
#include <string>

namespace ink::compiler::internal
{
class binary_reader
{
public:
	binary_reader(std::istream& input);
	uint8_t              read_uint8();
	uint16_t             read_uint16();
	int                  read_int();
	std::string          read_string();
	std::vector<uint8_t> read_bytes(size_t count);

private:
	std::istream& _input;
	bool          _swap_endian;
};
} // namespace ink::compiler::internal