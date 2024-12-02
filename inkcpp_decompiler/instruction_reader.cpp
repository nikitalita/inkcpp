
#include "instruction_reader.h"
#include "story_impl.h"

#include <vector>

namespace ink
{
namespace decompiler
{
	const char* instruction_info::get_string_param(runtime::internal::story_impl* story) const
	{
		return story->string(param.uint_param);
	}

	template<typename T>
	inline T instruction_reader::read()
	{
		using header = ink::internal::header;
		// Sanity
		if (_ptr + sizeof(T) <= _end) {
			inkAssert(_ptr + sizeof(T) <= _end, "Unexpected EOF in Ink execution");
		}

		// Read memory
		T val = *( const T* ) _ptr;
		if (_endianess == header::endian_types::differ) {
			val = header::swap_bytes(val);
		}

		// Advance ip
		_ptr += sizeof(T);

		// Return
		return val;
	}

	instruction_info instruction_reader::read_instruction()
	{
		inkAssert(! at_end());
		instruction_info instruction;
		instruction.offset  = static_cast<uint32_t>(_ptr - _start);
		auto cmd = read<uint8_t>();
		if (_binVersion == 0) {
			instruction.command = convert_old_to_new_cmd(static_cast<CommandVer0>(cmd));
		} else {
			instruction.command = static_cast<Command>(cmd);
		}
		instruction.flag    = read<uint8_t>();
		// Determine the number of parameters based on the command type
		switch (instruction.command) {
			case Command::TAG:
			case Command::STR: {
				instruction.param.uint_param = read<uint32_t>();
			} break;
			case Command::INT: {
				instruction.param.int_param = read<int>();
			} break;
			case Command::BOOL: {
				instruction.param.bool_param = read<int>() ? true : false;
				break;
			}
			case Command::FLOAT: {
				instruction.param.float_param = read<float>();
				break;
			}
			case Command::VALUE_POINTER:
			case Command::LIST: {
				instruction.param.hash_param = read<hash_t>();
			} break;
			case Command::DIVERT_VAL:
			case Command::DIVERT:
			case Command::DIVERT_TO_VARIABLE:
			case Command::TUNNEL:
			case Command::FUNCTION:
			case Command::DEFINE_TEMP:
			case Command::SET_VARIABLE:
			case Command::PUSH_VARIABLE_VALUE:
			case Command::READ_COUNT:
			case Command::CHOICE:
			case Command::END_CONTAINER_MARKER:
			case Command::START_CONTAINER_MARKER: {
				instruction.param.uint_param = read<uint32_t>();
			} break;
			case Command::CALL_EXTERNAL: {
				instruction.param.uint_param = read<uint32_t>();
				if (_binVersion > 0) {
					// pop off the following function call
					read<uint8_t>();
					read<uint8_t>();
					instruction.addtl_param = read<uint32_t>();
				}
			} break;
			// None of these have parameters
			case Command::NEWLINE:
			case Command::GLUE:
			case Command::DONE:
			case Command::END:
			case Command::START_EVAL:
			case Command::END_EVAL:
			case Command::OUTPUT:
			case Command::POP:
			case Command::DUPLICATE:
			case Command::VISIT:
			case Command::TURN:
			case Command::SEQUENCE:
			case Command::SEED:
			case Command::START_STR:
			case Command::END_STR:
			case Command::START_TAG:
			case Command::END_TAG:
			case Command::THREAD:
			case Command::LIST_RANGE:
			case Command::ADD:
			case Command::SUBTRACT:
			case Command::DIVIDE:
			case Command::MULTIPLY:
			case Command::MOD:
			case Command::RANDOM:
			case Command::IS_EQUAL:
			case Command::GREATER_THAN:
			case Command::LESS_THAN:
			case Command::GREATER_THAN_EQUALS:
			case Command::LESS_THAN_EQUALS:
			case Command::NOT_EQUAL:
			case Command::AND:
			case Command::OR:
			case Command::MIN:
			case Command::MAX:
			case Command::HAS:
			case Command::HASNT:
			case Command::INTERSECTION:
			case Command::LIST_INT:
			case Command::NOT:
			case Command::NEGATE:
			case Command::LIST_COUNT:
			case Command::LIST_MIN:
			case Command::LIST_MAX:
			case Command::READ_COUNT_VAR:
			case Command::TURNS:
			case Command::lrnd:
			case Command::FLOOR:
			case Command::CEILING:
			case Command::INT_CAST:
			case Command::LIST_ALL:
			case Command::LIST_INVERT:
			case Command::LIST_VALUE:
			case Command::CHOICE_COUNT:
			case Command::TUNNEL_RETURN:
			case Command::VOID:
			case Command::FUNCTION_RETURN: {
			} break;
			default: {
				printf("Unknown command: %u\n", static_cast<uint8_t>(instruction.command));
			} break;
		}
		return instruction;
	}
	
	Command instruction_reader::convert_old_to_new_cmd(CommandVer0 cmd)
	{
	    switch (cmd)
	    {
	        case CommandVer0::STR: return Command::STR;
	        case CommandVer0::INT: return Command::INT;
	        case CommandVer0::BOOL: return Command::BOOL;
	        case CommandVer0::FLOAT: return Command::FLOAT;
	        case CommandVer0::VALUE_POINTER: return Command::VALUE_POINTER;
	        case CommandVer0::DIVERT_VAL: return Command::DIVERT_VAL;
	        case CommandVer0::LIST: return Command::LIST;
	        case CommandVer0::NEWLINE: return Command::NEWLINE;
	        case CommandVer0::GLUE: return Command::GLUE;
	        case CommandVer0::VOID: return Command::VOID;
	        case CommandVer0::TAG: return Command::TAG;
	        case CommandVer0::DIVERT: return Command::DIVERT;
	        case CommandVer0::DIVERT_TO_VARIABLE: return Command::DIVERT_TO_VARIABLE;
	        case CommandVer0::TUNNEL: return Command::TUNNEL;
	        case CommandVer0::FUNCTION: return Command::FUNCTION;
	        case CommandVer0::DONE: return Command::DONE;
	        case CommandVer0::END: return Command::END;
	        case CommandVer0::TUNNEL_RETURN: return Command::TUNNEL_RETURN;
	        case CommandVer0::FUNCTION_RETURN: return Command::FUNCTION_RETURN;
	        case CommandVer0::DEFINE_TEMP: return Command::DEFINE_TEMP;
	        case CommandVer0::SET_VARIABLE: return Command::SET_VARIABLE;
	        case CommandVer0::START_EVAL: return Command::START_EVAL;
	        case CommandVer0::END_EVAL: return Command::END_EVAL;
	        case CommandVer0::OUTPUT: return Command::OUTPUT;
	        case CommandVer0::POP: return Command::POP;
	        case CommandVer0::DUPLICATE: return Command::DUPLICATE;
	        case CommandVer0::PUSH_VARIABLE_VALUE: return Command::PUSH_VARIABLE_VALUE;
	        case CommandVer0::VISIT: return Command::VISIT;
	        case CommandVer0::READ_COUNT: return Command::READ_COUNT;
	        case CommandVer0::SEQUENCE: return Command::SEQUENCE;
	        case CommandVer0::SEED: return Command::SEED;
	        case CommandVer0::START_STR: return Command::START_STR;
	        case CommandVer0::END_STR: return Command::END_STR;
	        case CommandVer0::CHOICE: return Command::CHOICE;
	        case CommandVer0::THREAD: return Command::THREAD;
	        case CommandVer0::LIST_RANGE: return Command::LIST_RANGE;
	        case CommandVer0::ADD: return Command::ADD;
	        case CommandVer0::SUBTRACT: return Command::SUBTRACT;
	        case CommandVer0::DIVIDE: return Command::DIVIDE;
	        case CommandVer0::MULTIPLY: return Command::MULTIPLY;
	        case CommandVer0::MOD: return Command::MOD;
	        case CommandVer0::RANDOM: return Command::RANDOM;
	        case CommandVer0::IS_EQUAL: return Command::IS_EQUAL;
	        case CommandVer0::GREATER_THAN: return Command::GREATER_THAN;
	        case CommandVer0::LESS_THAN: return Command::LESS_THAN;
	        case CommandVer0::GREATER_THAN_EQUALS: return Command::GREATER_THAN_EQUALS;
	        case CommandVer0::LESS_THAN_EQUALS: return Command::LESS_THAN_EQUALS;
	        case CommandVer0::NOT_EQUAL: return Command::NOT_EQUAL;
	        case CommandVer0::AND: return Command::AND;
	        case CommandVer0::OR: return Command::OR;
	        case CommandVer0::MIN: return Command::MIN;
	        case CommandVer0::MAX: return Command::MAX;
	        case CommandVer0::HAS: return Command::HAS;
	        case CommandVer0::HASNT: return Command::HASNT;
	        case CommandVer0::INTERSECTION: return Command::INTERSECTION;
	        case CommandVer0::LIST_INT: return Command::LIST_INT;
	        case CommandVer0::NOT: return Command::NOT;
	        case CommandVer0::NEGATE: return Command::NEGATE;
	        case CommandVer0::LIST_COUNT: return Command::LIST_COUNT;
	        case CommandVer0::LIST_MIN: return Command::LIST_MIN;
	        case CommandVer0::LIST_MAX: return Command::LIST_MAX;
	        case CommandVer0::READ_COUNT_VAR: return Command::READ_COUNT_VAR;
	        case CommandVer0::TURNS: return Command::TURNS;
	        case CommandVer0::lrnd: return Command::lrnd;
	        case CommandVer0::FLOOR: return Command::FLOOR;
	        case CommandVer0::CEILING: return Command::CEILING;
	        case CommandVer0::INT_CAST: return Command::INT_CAST;
	        case CommandVer0::LIST_ALL: return Command::LIST_ALL;
	        case CommandVer0::LIST_INVERT: return Command::LIST_INVERT;
	        case CommandVer0::LIST_VALUE: return Command::LIST_VALUE;
	        case CommandVer0::CHOICE_COUNT: return Command::CHOICE_COUNT;
	        case CommandVer0::START_CONTAINER_MARKER: return Command::START_CONTAINER_MARKER;
	        case CommandVer0::END_CONTAINER_MARKER: return Command::END_CONTAINER_MARKER;
	        case CommandVer0::CALL_EXTERNAL: return Command::CALL_EXTERNAL;
	        default: return Command::NUM_COMMANDS;
	    }
	}
	
	CommandVer0 instruction_reader::convert_new_to_old_cmd(Command cmd)
	{
    switch (cmd)
    {
        case Command::STR: return CommandVer0::STR;
        case Command::INT: return CommandVer0::INT;
        case Command::BOOL: return CommandVer0::BOOL;
        case Command::FLOAT: return CommandVer0::FLOAT;
        case Command::VALUE_POINTER: return CommandVer0::VALUE_POINTER;
        case Command::DIVERT_VAL: return CommandVer0::DIVERT_VAL;
        case Command::LIST: return CommandVer0::LIST;
        case Command::NEWLINE: return CommandVer0::NEWLINE;
        case Command::GLUE: return CommandVer0::GLUE;
        case Command::VOID: return CommandVer0::VOID;
        case Command::TAG: return CommandVer0::TAG;
        case Command::DIVERT: return CommandVer0::DIVERT;
        case Command::DIVERT_TO_VARIABLE: return CommandVer0::DIVERT_TO_VARIABLE;
        case Command::TUNNEL: return CommandVer0::TUNNEL;
        case Command::FUNCTION: return CommandVer0::FUNCTION;
        case Command::DONE: return CommandVer0::DONE;
        case Command::END: return CommandVer0::END;
        case Command::TUNNEL_RETURN: return CommandVer0::TUNNEL_RETURN;
        case Command::FUNCTION_RETURN: return CommandVer0::FUNCTION_RETURN;
        case Command::DEFINE_TEMP: return CommandVer0::DEFINE_TEMP;
        case Command::SET_VARIABLE: return CommandVer0::SET_VARIABLE;
        case Command::START_EVAL: return CommandVer0::START_EVAL;
        case Command::END_EVAL: return CommandVer0::END_EVAL;
        case Command::OUTPUT: return CommandVer0::OUTPUT;
        case Command::POP: return CommandVer0::POP;
        case Command::DUPLICATE: return CommandVer0::DUPLICATE;
        case Command::PUSH_VARIABLE_VALUE: return CommandVer0::PUSH_VARIABLE_VALUE;
        case Command::VISIT: return CommandVer0::VISIT;
        case Command::READ_COUNT: return CommandVer0::READ_COUNT;
        case Command::SEQUENCE: return CommandVer0::SEQUENCE;
        case Command::SEED: return CommandVer0::SEED;
        case Command::START_STR: return CommandVer0::START_STR;
        case Command::END_STR: return CommandVer0::END_STR;
        case Command::CHOICE: return CommandVer0::CHOICE;
        case Command::THREAD: return CommandVer0::THREAD;
        case Command::LIST_RANGE: return CommandVer0::LIST_RANGE;
        case Command::ADD: return CommandVer0::ADD;
        case Command::SUBTRACT: return CommandVer0::SUBTRACT;
        case Command::DIVIDE: return CommandVer0::DIVIDE;
        case Command::MULTIPLY: return CommandVer0::MULTIPLY;
        case Command::MOD: return CommandVer0::MOD;
        case Command::RANDOM: return CommandVer0::RANDOM;
        case Command::IS_EQUAL: return CommandVer0::IS_EQUAL;
        case Command::GREATER_THAN: return CommandVer0::GREATER_THAN;
        case Command::LESS_THAN: return CommandVer0::LESS_THAN;
        case Command::GREATER_THAN_EQUALS: return CommandVer0::GREATER_THAN_EQUALS;
        case Command::LESS_THAN_EQUALS: return CommandVer0::LESS_THAN_EQUALS;
        case Command::NOT_EQUAL: return CommandVer0::NOT_EQUAL;
        case Command::AND: return CommandVer0::AND;
        case Command::OR: return CommandVer0::OR;
        case Command::MIN: return CommandVer0::MIN;
        case Command::MAX: return CommandVer0::MAX;
        case Command::HAS: return CommandVer0::HAS;
        case Command::HASNT: return CommandVer0::HASNT;
        case Command::INTERSECTION: return CommandVer0::INTERSECTION;
        case Command::LIST_INT: return CommandVer0::LIST_INT;
        case Command::NOT: return CommandVer0::NOT;
        case Command::NEGATE: return CommandVer0::NEGATE;
        case Command::LIST_COUNT: return CommandVer0::LIST_COUNT;
        case Command::LIST_MIN: return CommandVer0::LIST_MIN;
        case Command::LIST_MAX: return CommandVer0::LIST_MAX;
        case Command::READ_COUNT_VAR: return CommandVer0::READ_COUNT_VAR;
        case Command::TURNS: return CommandVer0::TURNS;
        case Command::lrnd: return CommandVer0::lrnd;
        case Command::FLOOR: return CommandVer0::FLOOR;
        case Command::CEILING: return CommandVer0::CEILING;
        case Command::INT_CAST: return CommandVer0::INT_CAST;
        case Command::LIST_ALL: return CommandVer0::LIST_ALL;
        case Command::LIST_INVERT: return CommandVer0::LIST_INVERT;
        case Command::LIST_VALUE: return CommandVer0::LIST_VALUE;
        case Command::CHOICE_COUNT: return CommandVer0::CHOICE_COUNT;
        case Command::START_CONTAINER_MARKER: return CommandVer0::START_CONTAINER_MARKER;
        case Command::END_CONTAINER_MARKER: return CommandVer0::END_CONTAINER_MARKER;
        case Command::CALL_EXTERNAL: return CommandVer0::CALL_EXTERNAL;
        default: return CommandVer0::NUM_COMMANDS;
    }

	}

	

	void instruction_reader::write_instruction( int binVersion,
    const instruction_info& instruction, std::ostream& os
)
{
		auto cmd = static_cast<uint8_t>(instruction.command);
		if (binVersion == 0) {
			cmd = static_cast<uint8_t>(convert_new_to_old_cmd(instruction.command));
		}
    os.put(cmd);
    os.put(instruction.flag);
    switch (instruction.command) {
        case Command::TAG:
        case Command::STR: {
            os.write(reinterpret_cast<const char*>(&instruction.param.uint_param), sizeof(uint32_t));
        } break;
        case Command::INT: {
            os.write(reinterpret_cast<const char*>(&instruction.param.int_param), sizeof(int));
        } break;
        case Command::BOOL: {
            os.write(reinterpret_cast<const char*>(&instruction.param.bool_param), sizeof(int));
            break;
        }
        case Command::FLOAT: {
            os.write(reinterpret_cast<const char*>(&instruction.param.float_param), sizeof(float));
            break;
        }
        case Command::VALUE_POINTER:
        case Command::LIST: {
            os.write(reinterpret_cast<const char*>(&instruction.param.hash_param), sizeof(hash_t));
        } break;
        case Command::DIVERT_VAL:
        case Command::DIVERT:
        case Command::DIVERT_TO_VARIABLE:
        case Command::TUNNEL:
        case Command::FUNCTION:
        case Command::DEFINE_TEMP:
        case Command::SET_VARIABLE:
        case Command::PUSH_VARIABLE_VALUE:
        case Command::READ_COUNT:
        case Command::CHOICE:
        case Command::END_CONTAINER_MARKER:
        case Command::START_CONTAINER_MARKER: {
            os.write(reinterpret_cast<const char*>(&instruction.param.uint_param), sizeof(uint32_t));
        } break;
        case Command::CALL_EXTERNAL: {
            os.write(reinterpret_cast<const char*>(&instruction.param.uint_param), sizeof(uint32_t));
        	if (binVersion > 0) {
        		os.put(static_cast<uint8_t>(Command::FUNCTION));
        		os.put(static_cast<uint8_t>(CommandFlag::FALLBACK_FUNCTION));
        		os.write(reinterpret_cast<const char*>(&instruction.addtl_param), sizeof(uint32_t));
        	}
        } break;
        // None of these have parameters
        case Command::NEWLINE:
        case Command::GLUE:
        case Command::DONE:
        case Command::END:
        case Command::START_EVAL:
        case Command::END_EVAL:
        case Command::OUTPUT:
        case Command::POP:
        case Command::DUPLICATE:
        case Command::VISIT:
        case Command::TURN:
        case Command::SEQUENCE:
        case Command::SEED:
        case Command::START_STR:
        case Command::END_STR:
        case Command::START_TAG:
        case Command::END_TAG:
        case Command::THREAD:
        case Command::LIST_RANGE:
        case Command::ADD:
        case Command::SUBTRACT:
        case Command::DIVIDE:
        case Command::MULTIPLY:
        case Command::MOD:
        case Command::RANDOM:
        case Command::IS_EQUAL:
        case Command::GREATER_THAN:
        case Command::LESS_THAN:
        case Command::GREATER_THAN_EQUALS:
        case Command::LESS_THAN_EQUALS:
        case Command::NOT_EQUAL:
        case Command::AND:
        case Command::OR:
        case Command::MIN:
        case Command::MAX:
        case Command::HAS:
        case Command::HASNT:
        case Command::INTERSECTION:
        case Command::LIST_INT:
        case Command::NOT:
        case Command::NEGATE:
        case Command::LIST_COUNT:
        case Command::LIST_MIN:
        case Command::LIST_MAX:
        case Command::READ_COUNT_VAR:
        case Command::TURNS:
        case Command::lrnd:
        case Command::FLOOR:
        case Command::CEILING:
        case Command::INT_CAST:
        case Command::LIST_ALL:
        case Command::LIST_INVERT:
        case Command::LIST_VALUE:
        case Command::CHOICE_COUNT:
        case Command::TUNNEL_RETURN:
        case Command::VOID:
        case Command::FUNCTION_RETURN: {
        } break;
        default: {
					printf("Unknown command: %u\n", static_cast<uint8_t>(instruction.command));
        } break;
    }
}


	bool instruction_reader::at_end() const { return _ptr >= _end; }

	ip_t instruction_reader::get_pos() const { return _ptr; }

	instruction_reader::instruction_reader(const runtime::internal::story_impl* story)
	    : instruction_reader(story->get_header().ink_bin_version_number, story->instructions(), story->end(), story->string(0), story->get_header().endien)
	{
	}

	instruction_reader::instruction_reader( int binVersion,
	    const ip_t start, const ip_t end, const char* _string_table,
	    ink::internal::header::endian_types endianness
	)
		: _binVersion(binVersion)
	    , _ptr(start)
	    , _start(start)
	    , _end(end)
	    , _string_table(_string_table)
	    , _endianess(endianness)
	{
	}

} // namespace decompiler
} // namespace ink