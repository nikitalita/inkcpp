#include "reverse_compiler.h"
#include "command.h"

namespace ink::compiler::internal
{
reverse_compiler::reverse_compiler(
    binary_reader& reader, json_emitter& emitter, compilation_results* results
)
    : _reader(reader)
    , _emitter(emitter)
    , _results(results)
{
}

nlohmann::json reverse_compiler::read_parameter(ink::Command command)
{
	nlohmann::json parameter;

	switch (command) {
		case ink::Command::STR: {
			int string_index = _reader.read_int();
			parameter        = _strings[string_index];
			break;
		}
		case ink::Command::INT:
		case ink::Command::BOOL:
		case ink::Command::FLOAT:
		case ink::Command::VALUE_POINTER:
		case ink::Command::DIVERT_VAL: {
			int value = _reader.read_int();
			parameter = value;
			break;
		}
		case ink::Command::DIVERT:
		case ink::Command::DIVERT_TO_VARIABLE:
		case ink::Command::TUNNEL:
		case ink::Command::FUNCTION: {
			int target_index = _reader.read_int();
			parameter        = _strings[target_index];
			break;
		}
		case ink::Command::LIST_RANGE: {
			int list_index = _reader.read_int();
			int min_value  = _reader.read_int();
			int max_value  = _reader.read_int();
			parameter      = {
          {"list", _lists[list_index]},
          { "min",          min_value},
          { "max",          max_value}
      };
			break;
		}
		case ink::Command::LIST: {
			int list_index = _reader.read_int();
			parameter      = _lists[list_index];
			break;
		}
		default: {
			int value = _reader.read_int();
			parameter = value;
			break;
		}
	}

	return parameter;
}

std::vector<nlohmann::json> reverse_compiler::read_instructions()
{
	std::vector<nlohmann::json> instructions;
	while (true) {
		uint8_t command_index = _reader.read_uint8();
		if (command_index == 0xFF) // Assuming 0xFF is used to indicate the end of instructions
			break;

		uint8_t command_flag = _reader.read_uint8();

		nlohmann::json instruction;
		instruction["command"] = command_index;
		instruction["flag"]    = command_flag;

		// Determine the number of parameters based on the command type
		size_t param_count = 0;
		switch (static_cast<ink::Command>(command_index)) {
			case ink::Command::STR:
			case ink::Command::INT:
			case ink::Command::BOOL:
			case ink::Command::FLOAT:
			case ink::Command::VALUE_POINTER:
			case ink::Command::DIVERT_VAL:
			case ink::Command::LIST:
			case ink::Command::NEWLINE:
			case ink::Command::GLUE:
			case ink::Command::VOID:
			case ink::Command::TAG: param_count = 1; break;
			case ink::Command::DIVERT:
			case ink::Command::DIVERT_TO_VARIABLE:
			case ink::Command::TUNNEL:
			case ink::Command::FUNCTION: param_count = 2; break;
			case ink::Command::DONE:
			case ink::Command::END:
			case ink::Command::TUNNEL_RETURN:
			case ink::Command::FUNCTION_RETURN: param_count = 0; break;
			case ink::Command::DEFINE_TEMP:
			case ink::Command::SET_VARIABLE: param_count = 1; break;
			case ink::Command::START_EVAL:
			case ink::Command::END_EVAL:
			case ink::Command::OUTPUT:
			case ink::Command::POP:
			case ink::Command::DUPLICATE:
			case ink::Command::PUSH_VARIABLE_VALUE:
			case ink::Command::VISIT:
			case ink::Command::TURN:
			case ink::Command::READ_COUNT:
			case ink::Command::SEQUENCE:
			case ink::Command::SEED: param_count = 1; break;
			case ink::Command::START_STR:
			case ink::Command::END_STR:
			case ink::Command::START_TAG:
			case ink::Command::END_TAG: param_count = 0; break;
			case ink::Command::CHOICE: param_count = 3; break;
			case ink::Command::THREAD: param_count = 2; break;
			case ink::Command::LIST_RANGE:
			case ink::Command::ADD:
			case ink::Command::SUBTRACT:
			case ink::Command::DIVIDE:
			case ink::Command::MULTIPLY:
			case ink::Command::MOD:
			case ink::Command::RANDOM:
			case ink::Command::IS_EQUAL:
			case ink::Command::GREATER_THAN:
			case ink::Command::LESS_THAN:
			case ink::Command::GREATER_THAN_EQUALS:
			case ink::Command::LESS_THAN_EQUALS:
			case ink::Command::NOT_EQUAL:
			case ink::Command::AND:
			case ink::Command::OR:
			case ink::Command::MIN:
			case ink::Command::MAX:
			case ink::Command::HAS:
			case ink::Command::HASNT:
			case ink::Command::INTERSECTION:
			case ink::Command::LIST_INT: param_count = 2; break;
			case ink::Command::NOT:
			case ink::Command::NEGATE:
			case ink::Command::LIST_COUNT:
			case ink::Command::LIST_MIN:
			case ink::Command::LIST_MAX:
			case ink::Command::READ_COUNT_VAR:
			case ink::Command::TURNS:
			case ink::Command::lrnd:
			case ink::Command::FLOOR:
			case ink::Command::CEILING:
			case ink::Command::INT_CAST:
			case ink::Command::LIST_ALL:
			case ink::Command::LIST_INVERT:
			case ink::Command::LIST_VALUE: param_count = 1; break;
			case ink::Command::CHOICE_COUNT: param_count = 0; break;
			case ink::Command::START_CONTAINER_MARKER:
			case ink::Command::END_CONTAINER_MARKER: param_count = 1; break;
			case ink::Command::CALL_EXTERNAL: param_count = 2; break;
			default: param_count = 0; break;
		}

		for (size_t i = 0; i < param_count; ++i) {
			instruction["parameters"].push_back(read_parameter(static_cast<ink::Command>(command_index)));
		}

		instructions.push_back(instruction);
	}
	return instructions;
}

void reverse_compiler::compile()
{
	// Read Ink version
	int ink_version = _reader.read_int();
	_emitter.start(ink_version);

	// Read string list
	while (true) {
		std::string str = _reader.read_string();
		if (str.empty())
			break;
		_strings.push_back(str);
	}
	_emitter.write_list(_strings);

	// Read list definitions
	std::vector<nlohmann::json> list_definitions;
	while (true) {
		std::string flag_name = _reader.read_string();
		if (flag_name.empty())
			break;

		nlohmann::json list_flag;
		list_flag["flag_name"] = flag_name;
		list_flag["list_id"]   = _reader.read_int();
		list_flag["value"]     = _reader.read_int();
		list_definitions.push_back(list_flag);
	}
	_emitter.write_container(list_definitions);

	// Read lists
	std::vector<nlohmann::json> lists;
	while (true) {
		nlohmann::json list;
		while (true) {
			std::string flag_name = _reader.read_string();
			if (flag_name.empty())
				break;
			list.push_back(flag_name);
		}
		if (list.empty())
			break;
		lists.push_back(list);
	}
	_emitter.write_container(lists);

	// Read container instruction map
	std::vector<nlohmann::json> container_instruction_map;
	while (true) {
		int offset = _reader.read_int();
		if (offset == -1)
			break;

		nlohmann::json container_instruction;
		container_instruction["offset"]          = offset;
		container_instruction["container_index"] = _reader.read_int();
		container_instruction_map.push_back(container_instruction);
	}
	_emitter.write_container(container_instruction_map);

	// Read container hash map
	std::map<std::string, int> container_hash_map;
	while (true) {
		std::string container_name = _reader.read_string();
		if (container_name.empty())
			break;
		int offset                         = _reader.read_int();
		container_hash_map[container_name] = offset;
	}
	_emitter.write_container(container_hash_map);

	// Read instructions
	std::vector<nlohmann::json> instructions = read_instructions();
	_emitter.write_container(instructions);

	_emitter.finish();
}
} // namespace ink::compiler::internal