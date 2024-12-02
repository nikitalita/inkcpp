// old
#include "reverse_compiler.h"
#include "command.h"
#include "range_map/range_map.hpp"
#include "instruction_reader.h"

namespace ink::decompiler::internal
{
reverse_compiler::reverse_compiler(ink::runtime::internal::story_impl& story, json_emitter& emitter)
    : _story(story)
    , _emitter(emitter)
{
}


nlohmann::json reverse_compiler::serialize_instruction(const instruction_info& info) const
{
	nlohmann::json instruction;
	using ink::Command;
	// Determine the number of parameters based on the command type
	const char* command_string = info.command < Command::NUM_COMMANDS
	                               ? CommandStrings[static_cast<uint8_t>(info.command)]
	                               : "<INVALID_COMMAND>";
	auto length_of_instructions = _story.end() - _story.instructions();
	switch (info.command) {
		case Command::STR: {
			instruction = std::string("^") + info.get_string_param(&_story);
		} break;
		case Command::INT: {
			instruction = info.param.int_param;
		} break;
		case Command::BOOL: {
			instruction = info.param.bool_param;
			break;
		}
		case Command::FLOAT: {
			instruction = info.param.float_param;
			break;
		}
		case Command::VALUE_POINTER: {
			// TODO: hash lookup
			auto target = info.param.hash_param;
			instruction = {
			    {"^var",			                        target},
          {  "ci", static_cast<char>(info.flag) - 1}
			};
			break;
		}
		case Command::DIVERT_VAL: {
			auto target = info.param.uint_param;
			auto target_cmd = *(_story.instructions() + target);
			instruction       = {{"^->", "TODO: emit a named container before this instruction: " + std::string(CommandStrings[target_cmd]) + " @ " + std::to_string(target)}};
			break;
		}
		case Command::LIST: {
			auto list_index = info.param.hash_param;
			instruction    = "TODO: list " + std::to_string(list_index);
		} break;
		case Command::VOID: {
			instruction = "";
		}; break;
		case Command::TAG: {
			instruction = {
			    {'#', info.get_string_param(&_story)}
			};
		};
		case Command::DIVERT: {
			auto value   = info.param.uint_param;
			if (value == length_of_instructions) {
				return nullptr;
			}
			instruction = {
			    {"->", get_container_path(value)}
			};
			if (info.flag & ( uint8_t ) CommandFlag::DIVERT_HAS_CONDITION) {
				instruction["c"] = true;
			}
		} break;
		case Command::DIVERT_TO_VARIABLE: {
			auto value   = info.param.uint_param;
			auto str_value = "$r" + std::to_string(value);
			instruction = {
			    {"->", str_value},
					{"var", true},
			};
			if (info.flag & ( uint8_t ) CommandFlag::DIVERT_HAS_CONDITION) {
				instruction["c"] = true;
			}
		} break;
		case Command::TUNNEL: {
			auto value   = info.param.uint_param;
			instruction = {
			    {"->t->", get_container_path(value)}
			};
			if (info.flag & ( uint8_t ) CommandFlag::TUNNEL_TO_VARIABLE) {
				instruction["var"] = true;
			}
		} break;
		case Command::FUNCTION: {
			// f()
			auto value   = info.param.uint_param;
			// TODO: real val
			instruction = {
			    {"f()", value}
			};
			if (info.flag & ( uint8_t ) CommandFlag::FUNCTION_TO_VARIABLE) {
				instruction["var"] = true;
			}
		};
		case Command::DEFINE_TEMP: {
			auto value   = info.param.uint_param;
			auto str_value = "$r" + std::to_string(value);
			instruction = {
			    {"temp=", str_value}
			};
			if (info.flag & ( uint8_t ) CommandFlag::ASSIGNMENT_IS_REDEFINE) {
				instruction["re"] = true;
			}
		} break;
		case Command::SET_VARIABLE: {
			auto value   = info.param.uint_param;
			// TODO: Actual variable name
			instruction = {
			    {"VAR=", value}
			};
			if (info.flag & ( uint8_t ) CommandFlag::ASSIGNMENT_IS_REDEFINE) {
				instruction["re"] = true;
			}
		} break;
		case Command::PUSH_VARIABLE_VALUE: {
			auto value   = info.param.uint_param;
			// TODO: Actual variable name

			instruction = {
			    {"VAR?", value}
			};
		} break;
		case Command::READ_COUNT: {
			auto value   = info.param.uint_param;
			// TODO: Actual variable name
			instruction = {
			    {"CNT?", value}
			};
		} break;
		case Command::CHOICE: {
			auto value   = info.param.uint_param;
			instruction = {
			    {  "*",			                        get_container_path(value)},
          {"flg", static_cast<uint8_t>(info.flag)}
			};

		} break;
		case Command::CALL_EXTERNAL: {
			int numArgs = info.flag;
			auto hash    = info.param.uint_param;
			auto val     = info.addtl_param;
			// TODO: real val
			instruction = {
			    {   "x()",     val},
			    {"exArgs", numArgs},
			};
		} break;
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
		case Command::FUNCTION_RETURN:{
			instruction = command_string;
		} break;
		case Command::END_CONTAINER_MARKER:
		case Command::START_CONTAINER_MARKER: {
			// this is supposed to be handled down below
			printf("woopsie!");
		}
		default: {
			instruction = command_string;
			printf("Unknown command: %d\n", info.command);
		} break;
	}


	return instruction;
}

std::vector<nlohmann::json> reverse_compiler::decompile_instructions()
{
	std::vector<nlohmann::json> instructions;
	instruction_reader reader{&_story};
	while (!reader.at_end()) {
		container_t container_id;
		std::vector<container_t> container_ids;
		auto curr_pos = reader.get_pos();
		auto inst = reader.read_instruction();
		if (is_end_of_container(curr_pos, container_ids)) {
			uint32_t indexToReturn = 0;
			uint8_t command_flag = 0;
			if (inst.command == Command::END_CONTAINER_MARKER) {
				indexToReturn = inst.param.uint_param;
				command_flag = inst.flag;
			}
			for (int i = 0; i < container_ids.size(); i++) {
				end_container(indexToReturn, command_flag);
			}
			if (inst.command == Command::END_CONTAINER_MARKER) {
				continue;
			}
		}
		if (is_start_of_container(curr_pos, container_id)) {
			uint32_t indexToReturn = 0;
			uint8_t command_flag = 0;
			if (inst.command == Command::START_CONTAINER_MARKER) {
				indexToReturn = inst.param.uint_param;
				command_flag = inst.flag;
			}
			start_container(container_id, indexToReturn, command_flag);
			if (inst.command == Command::START_CONTAINER_MARKER) {
				continue;
			}
		}
		auto instruction = serialize_instruction(inst);
		if (instruction != nullptr) {
			_emitter.write_instruction(instruction, get_comment_for_instruction(inst));
			instructions.push_back(instruction);
		}
	}

	return instructions;
}

std::vector<uint32_t> reverse_compiler::read_divert_offsets() const
{
	std::vector<uint32_t> offsets;
	instruction_reader reader{&_story};
	auto push_offset = [&](uint32_t target)
	{
		if (std::count(offsets.begin(), offsets.end(), target) == 0) {
			offsets.push_back(target);
		}
	};
	while (!reader.at_end()) {
		container_t container_id;
		uint8_t command_index;
		uint8_t command_flag = 0;
		uint32_t indexToReturn = 0;
		auto inst = reader.read_instruction();
		switch(inst.command) {
			case Command::DIVERT: {
				push_offset(inst.param.uint_param);
			} break;
			case Command::CHOICE: {
				push_offset(inst.param.uint_param);
			} break;
			case Command::DIVERT_VAL: {
				auto target = inst.param.uint_param;
				// TODO: something with this
			} break;
			// TODO: others
			default: {
			} break;
		}
	}
	std::sort(offsets.begin(), offsets.end());
	return offsets;
}

void reverse_compiler::start_container(container_t container_id, uint32_t indexToReturn, uint32_t flags)
{
	// get the hash from the container hash table
	auto name = _container_info[container_id].named ? "container-" + std::to_string(container_id) : "";
	container_stack.push_back(container_id);
	_curr_container_id = container_id;
	std::string comment =
		"Container " + std::to_string(container_id) +
			", offset: " + std::to_string(_container_info[container_id].startOffset) +
				", endOffset: " + std::to_string(_container_info[container_id].endOffset) +
					", real: " + std::to_string(_container_info[container_id].real);
	_curr_container_level = _emitter.start_container(name, comment);
}

void reverse_compiler::end_container(uint32_t indexToReturn, uint32_t flags)
{
	_curr_container_level = _emitter.end_container();
	if (!container_stack.empty()) {
		container_stack.pop_back();
	} else {
		printf("WARNING: Empty container stack\n");
	}
	if (container_stack.size() == 0) {
		_curr_container_id = 0;
	} else {
		_curr_container_id = container_stack.back();
	}
}

void reverse_compiler::pop_container_info()
{
	struct container_list_item {
		uint32_t offset;
		uint32_t containerIndex;
	};

	struct container_hash_list_item {
		hash_t   nameHash;
		uint32_t offset;
	};

	auto                             thingy         = _story.get_container_list();
	auto                             thingy2        = const_cast<uint32_t*>(thingy);
	container_list_item*             list_ptr       = reinterpret_cast<container_list_item*>(thingy2);
	auto                             num_containers = _story.num_containers();
	std::vector<container_list_item> container_list_items;
	for (int i = 0; i < num_containers * 2; i++) {
		container_list_items.push_back(list_ptr[i]);
	}
	for (int i = 0; i < num_containers; i++) {
		container_info info;
		info.id          = i;
		info.startOffset = 0;
		info.endOffset   = 0;
		_container_info.push_back(info);
	}
	std::vector<container_info*> container_stack;
	for (int i = 0; i < num_containers * 2; i++) {
		auto& item    = container_list_items[i];
		auto  item_id = item.containerIndex;
		if (container_stack.empty() || container_stack.back()->id != item_id) {
			container_stack.push_back(&_container_info[item_id]);
			_container_info[item_id].startOffset    = item.offset;
			start_offset_container_map[item.offset] = item_id;
		} else {
			_container_info[item_id].endOffset    = item.offset;
			end_offset_container_map[item.offset] = {item_id};
			container_stack.pop_back();
		}
	}
	beneficii::range_map<uint32_t, container_t> container_range_map;
	uint32_t length_of_instructions = _story.end() -_story.instructions();
	const container_t ROOT_ID = 0xFFFFFFFF;
	container_range_map.insert(true, {0, length_of_instructions, ROOT_ID});
	for (int i = 0; i < num_containers; i++) {
		beneficii::range_map<uint32_t, container_t>::range_type range = {
			_container_info[i].startOffset,
			_container_info[i].endOffset,
			_container_info[i].id
		};
		auto ret = container_range_map.insert(true, range);
		if (!ret.second) {
			printf("FUCK!");
		}
	}
	container_hash_list_item* hash_list_ptr = reinterpret_cast<container_hash_list_item*>(
	    const_cast<hash_t*>(_story.get_container_hash_start())
	);
	container_hash_list_item* hash_list_end = reinterpret_cast<container_hash_list_item*>(
	    const_cast<hash_t*>(_story.get_container_hash_end())
	);

	std::vector<container_hash_list_item> hash_list;

	printf("***HASH INFO***\n");
	std::vector<std::pair<uint32_t, uint32_t>> orphaned_hashes;
	std::vector<uint32_t> unique_offsets;
	for (; hash_list_ptr < hash_list_end; hash_list_ptr++) {
		_hash_to_offset_map[hash_list_ptr->nameHash] = hash_list_ptr->offset;
		printf("Hash: %u, Offset: %u\n", hash_list_ptr->nameHash, hash_list_ptr->offset);
		auto id = start_offset_container_map.find(hash_list_ptr->offset) != start_offset_container_map.end() ? start_offset_container_map[hash_list_ptr->offset] : -1;
		if (id != -1) {
			_hash_to_id_map[hash_list_ptr->nameHash] = id;
		} else {
			orphaned_hashes.push_back({hash_list_ptr->offset, hash_list_ptr->nameHash});
			// Don't need this?
			// if (!std::count(unique_offsets.begin(), unique_offsets.end(), hash_list_ptr->offset)) {
			// 	unique_offsets.push_back(hash_list_ptr->offset);
			// }
		}
	}
	auto divert_offsets = read_divert_offsets();
	for (auto& offset : divert_offsets) {
		if (!std::count(unique_offsets.begin(), unique_offsets.end(), offset) && start_offset_container_map.find(offset) == start_offset_container_map.end()) {
			unique_offsets.push_back(offset);
		}
	}
	// sort it soley by the offset
	std::sort(unique_offsets.begin(), unique_offsets.end());
	std::sort(orphaned_hashes.begin(), orphaned_hashes.end(), [](auto& a, auto& b) { return a.first < b.first; });
	for (int i = 0; i < unique_offsets.size(); i++) {
		auto offset = unique_offsets[i];
		uint32_t next_offset = i + 1 < unique_offsets.size() ? unique_offsets[i + 1] : length_of_instructions;
		uint32_t endOffset = next_offset;
		auto stack = container_range_map.find_ranges(offset);

		// get the top of the stack
		if (stack.size() == 0) {
			// Jump to the end, continue
			if (offset == length_of_instructions) {
				continue;
			} else {
				printf("ERROR: Orphaned offset: %u\n", offset);
				continue;
			}
		} else {
			beneficii::range_map<uint32_t, container_t>::iterator top = stack.top();
			beneficii::range_map<uint32_t, container_t>::iterator upper_bound = container_range_map.upper_bound(offset);
			if (upper_bound->range().get_left() < endOffset && upper_bound->range().get_right() > endOffset && top->range().get_left() <= upper_bound->range().get_left() && top->range().get_right() > upper_bound->range().get_right()) {
				endOffset = upper_bound->range().get_left();
			}  else if (top->range().get_right() < endOffset) {
				endOffset = top->range().get_right() - 4;
			}
		}
		container_info info;
		info.id = _container_info.size();
		info.named = std::count(divert_offsets.begin(), divert_offsets.end(), offset) != 0;
		info.real = false;
		info.startOffset = offset;
		info.endOffset = endOffset;
		_container_info.push_back(info);
		start_offset_container_map[offset] = info.id;
		if (end_offset_container_map.find(endOffset) == end_offset_container_map.end()) {
			end_offset_container_map[endOffset] = {info.id};
		} else {
			end_offset_container_map[endOffset].push_back(info.id);
		}
		container_range_map.insert(true, {offset, endOffset, info.id});
		auto it = std::find_if(orphaned_hashes.begin(), orphaned_hashes.end(), [offset](auto& a) { return a.first == offset; });
		for (; it != orphaned_hashes.end(); it++) {
			_hash_to_id_map[it->second] = info.id;
		}
	}
	int i = 0;
	printf("***CONTAINER INFO***\n");
	for (auto& container : _container_info) {
		printf("Container %u: Start: %u, End: %u\n", container.id, container.startOffset, container.endOffset);
	}

}

std::string reverse_compiler::get_container_path(const uint32_t offset) const
{
	// TODO: need to do relative/absolute paths
	if (start_offset_container_map.find(offset) != start_offset_container_map.end()) {
		const auto id = start_offset_container_map.at(offset);
		return "container-" + std::to_string(id);
	}
	return "<ANONYMOUS_CONTAINER @ OFFSET " + std::to_string(offset) + ">";
}

std::string reverse_compiler::get_comment_for_instruction(const instruction_info& inst) const
{
	std::string comment = "offset: " + std::to_string(inst.offset);
	// We're only doing the ones that actually have params
	switch(inst.command) {
		case Command::STR: {
			comment += " STR: " + std::to_string(inst.param.uint_param);
		} break;
		case Command::INT: {
			comment += " INT: " + std::to_string(inst.param.int_param);
		} break;
		case Command::BOOL: {
			comment += " BOOL: " + std::to_string(inst.param.bool_param);
		} break;
		case Command::FLOAT: {
			comment += " FLOAT: " + std::to_string(inst.param.float_param);
		} break;
		case Command::VALUE_POINTER: {
			comment += " VALUE_POINTER: " + std::to_string(inst.param.hash_param);
		} break;
		case Command::DIVERT_VAL: {
			comment += " DIVERT_VAL: " + std::to_string(inst.param.uint_param);
		} break;
		case Command::LIST: {
			comment += " LIST: " + std::to_string(inst.param.hash_param);
		} break;
		case Command::VOID: {
			comment += " VOID";
		} break;
		case Command::TAG: {
			comment += std::string(" TAG: ") + inst.get_string_param(&_story);
		} break;
		case Command::DIVERT: {
			comment += " DIVERT: " + std::to_string(inst.param.uint_param);
		} break;
		case Command::DIVERT_TO_VARIABLE: {
			comment += " DIVERT_TO_VARIABLE: " + std::to_string(inst.param.uint_param);
		} break;
		case Command::TUNNEL: {
			comment += " TUNNEL: " + std::to_string(inst.param.uint_param);
		} break;
		case Command::FUNCTION: {
			comment += " FUNCTION: " + std::to_string(inst.param.uint_param);
		} break;
		case Command::DEFINE_TEMP: {
			comment += " DEFINE_TEMP: " + std::to_string(inst.param.uint_param);
		} break;
		case Command::SET_VARIABLE: {
			comment += " SET_VARIABLE: " + std::to_string(inst.param.uint_param);
		} break;
		case Command::PUSH_VARIABLE_VALUE: {
			comment += " PUSH_VARIABLE_VALUE: " + std::to_string(inst.param.uint_param);
		} break;
		case Command::READ_COUNT: {
			comment += " READ_COUNT: " + std::to_string(inst.param.uint_param);
		} break;
		case Command::CHOICE: {
			comment += " CHOICE: " + std::to_string(inst.param.uint_param);
		} break;
		case Command::CALL_EXTERNAL: {
			comment += " CALL_EXTERNAL: " + std::to_string(inst.param.uint_param);
		} break;
		default:
			break;
	}
	return comment;
}

const reverse_compiler::container_info&
    reverse_compiler::get_container_info(const container_t container_id) const
{
	return _container_info[container_id];
}

reverse_compiler::container_info& reverse_compiler::get_container_info(container_t container_id)
{
	return _container_info[container_id];
}

bool reverse_compiler::is_start_of_container(ip_t pos, container_t& container_id) const
{
	uint32_t offset = pos - _story.instructions();
	if (start_offset_container_map.find(offset) != start_offset_container_map.end()) {
		container_id = start_offset_container_map.at(offset);
		return true;
	}
	return false;
}

bool reverse_compiler::is_end_of_container(ip_t pos, std::vector<container_t>& container_id) const
{
	uint32_t offset = pos - _story.instructions();
	if (end_offset_container_map.find(offset) != end_offset_container_map.end()) {
		container_id = end_offset_container_map.at(offset);
		return true;
	}
	return false;
}

void reverse_compiler::decompile()
{
	// Read Ink version
	_emitter.start(_story.get_header().ink_version_number);

	pop_container_info();
	// iterate through the containers in _story to get the container info
	
	

	// Read instructions
	decompile_instructions();

	_emitter.finish();
}
} // namespace ink::decompiler::internal