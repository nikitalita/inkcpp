
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
	enum class CommandVer0 : unsigned char
	{
		// == Value Commands: Push values onto the stack
		STR,
		INT,
		BOOL,
		FLOAT,
		VALUE_POINTER,
		DIVERT_VAL,
		LIST,
		NEWLINE,
		GLUE,
		VOID,
		TAG,

		// == Diverts
		DIVERT,
		DIVERT_TO_VARIABLE,
		TUNNEL,
		FUNCTION,

		// == Terminal commands
		DONE,
		END,
		TUNNEL_RETURN,
		FUNCTION_RETURN,

		// == Variable definitions
		DEFINE_TEMP,
		SET_VARIABLE,

		// == Evaluation stack
		START_EVAL,
		END_EVAL,
		OUTPUT,
		POP,
		DUPLICATE,
		PUSH_VARIABLE_VALUE,
		VISIT,
		READ_COUNT,
		SEQUENCE,
		SEED,

		// == String stack
		START_STR,
		END_STR,

		// == Choice commands
		CHOICE,

		// == Threading
		THREAD,
		// == thinary operations
		LIST_RANGE,
		OP_BEGIN = LIST_RANGE,
		TERNARY_OPERATORS_START = LIST_RANGE,
		TERNARY_OPERATORS_END = LIST_RANGE,
		// == Binary operators
		ADD,
		BINARY_OPERATORS_START = ADD,
		SUBTRACT,
		DIVIDE,
		MULTIPLY,
		MOD,
		RANDOM,
		IS_EQUAL,
		GREATER_THAN,
		LESS_THAN,
		GREATER_THAN_EQUALS,
		LESS_THAN_EQUALS,
		NOT_EQUAL,
		AND,
		OR,
		MIN,
		MAX,
		HAS,
		HASNT,
		INTERSECTION,
		LIST_INT,
		BINARY_OPERATORS_END = LIST_INT,

		// == Unary operators
		UNARY_OPERATORS_START,
		NOT = UNARY_OPERATORS_START,
		NEGATE,
		LIST_COUNT,
		LIST_MIN,
		LIST_MAX,
		READ_COUNT_VAR,
		TURNS,
		lrnd,
		FLOOR,
		CEILING,
		INT_CAST,
		LIST_ALL,
		LIST_INVERT,
		LIST_VALUE,
		UNARY_OPERATORS_END = LIST_VALUE,
		CHOICE_COUNT,
		OP_END,

		// == Container tracking
		START_CONTAINER_MARKER = OP_END,
		END_CONTAINER_MARKER,

		// == Function calls
		CALL_EXTERNAL,

		NUM_COMMANDS,
	};

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
		const char* get_string_param(const char * string_table) const;

		const char* get_string_param(runtime::internal::story_impl* story) const;
	};

	class instruction_reader
	{
	protected:
		template<typename T>
		inline T                       read();
		int _binVersion;
		ip_t                           _ptr;
		const ip_t                     _start;
		const ip_t                     _end;
		const char * 								 _string_table;
		ink::internal::header::endian_types _endianess;

	public:
		explicit instruction_reader(const runtime::internal::story_impl* story);
		instruction_reader( int binVersion,
		             ip_t start, ip_t end, const char* _string_table,
		             ink::internal::header::endian_types endianness
		         );

		instruction_info read_instruction();
		static Command convert_old_to_new_cmd(CommandVer0 cmd);
		static CommandVer0 convert_new_to_old_cmd(Command cmd);
		static void write_instruction(int binVersion, const instruction_info& instruction, std::ostream& os);
		bool             at_end() const;
		ip_t             get_pos() const;
	};


} // namespace decompiler
} // namespace ink