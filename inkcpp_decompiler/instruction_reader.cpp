
#include "instruction_reader.h"
#include "story_impl.h"

#include <vector>

namespace ink
{
namespace decompiler
{

	// instruction_info::instruction_info() { param.uint_param = 0; }

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
		inkAssert(!at_end());
		instruction_info instruction;
		instruction.offset  = static_cast<uint32_t>(_ptr - _start);
		instruction.command = static_cast<Command>(read<uint8_t>());
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
				// pop off the following function call
				read<uint8_t>();
				read<uint8_t>();
				instruction.addtl_param = read<uint32_t>();
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
				printf("Unknown command: %u\n", instruction.command);
			} break;
		}
		return instruction;
	}



	void instruction_reader::write_instruction(
    const instruction_info& instruction, std::ostream& os
)
{
    os.put(static_cast<uint8_t>(instruction.command));
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
            os.put(static_cast<uint8_t>(Command::FUNCTION));
            os.put(static_cast<uint8_t>(CommandFlag::FALLBACK_FUNCTION));
            os.write(reinterpret_cast<const char*>(&instruction.addtl_param), sizeof(uint32_t));
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
            printf("Unknown command: %u\n", instruction.command);
        } break;
    }
}


	bool instruction_reader::at_end() const { return _ptr >= _end; }

	ip_t instruction_reader::get_pos() const { return _ptr; }

	instruction_reader::instruction_reader(const runtime::internal::story_impl* story)
	    : instruction_reader(story->instructions(), story->end(), story->string(0), story->get_header().endien)
	{
	}

	instruction_reader::instruction_reader(
	    const ip_t start, const ip_t end, const char* _string_table,
	    ink::internal::header::endian_types endianness
	)
	    : _ptr(start)
	    , _start(start)
	    , _end(end)
	    , _string_table(_string_table)
	    , _endianess(endianness)
	{
	}

} // namespace decompiler
} // namespace ink