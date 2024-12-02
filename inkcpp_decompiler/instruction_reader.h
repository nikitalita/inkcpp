
#pragma once


#include <command.h>
#include <header.h>
#include <system.h>

namespace ink::runtime::internal
{
class story_impl;
} // namespace ink::runtime::internal

namespace ink
{
namespace decompiler
{
	struct instruction_info {
		uint32_t offset  = 0;
		Command  command = Command::NUM_COMMANDS;
		uint8_t  flag    = 0;

		union param_t {
			uint32_t uint_param;
			hash_t   hash_param;
			int      int_param;
			bool     bool_param;
			float    float_param;
		} param = {0};

		uint32_t    addtl_param = 0;
		const char* get_string_param(runtime::internal::story_impl* story) const;
	};

	class instruction_reader
	{
	protected:
		template<typename T>
		inline T                       read();
		ip_t                           _ptr;
		const ip_t                     _start;
		const ip_t                     _end;
		const char * 								 _string_table;
		ink::internal::header::endian_types _endianess;

	public:
		explicit instruction_reader(const runtime::internal::story_impl* story);
		instruction_reader(
		             ip_t start, ip_t end, const char* _string_table,
		             ink::internal::header::endian_types endianness
		         );

		instruction_info read_instruction();
		static void write_instruction(const instruction_info& instruction, std::ostream& os);
		bool             at_end() const;
		ip_t             get_pos() const;
	};


} // namespace decompiler
} // namespace ink