// old
#include "reverse_compiler.h"
#include "command.h"
#include "range_map/range_map.hpp"

namespace ink::decompiler::internal
{
reverse_compiler::reverse_compiler(ink::runtime::internal::story_impl& story, json_emitter& emitter)
    : _story(story)
    , _emitter(emitter)
{
	_ptr = _story.instructions();
}

template<typename T>
inline T reverse_compiler::read()
{
	using header = ink::internal::header;
	// Sanity
	if (_ptr + sizeof(T) <= _story.end()) {
		inkAssert(_ptr + sizeof(T) <= _story.end(), "Unexpected EOF in Ink execution");
	}

	// Read memory
	T val = *( const T* ) _ptr;
	if (_story.get_header().endien == header::endian_types::differ) {
		val = header::swap_bytes(val);
	}

	// Advance ip
	_ptr += sizeof(T);

	// Return
	return val;
}

template<>
inline const char* reverse_compiler::read()
{
	return read_string();
}

const char* reverse_compiler::read_string()
{
	offset_t str   = read<offset_t>();
	auto     strng = _story.string(str);
	return strng;
}

nlohmann::json reverse_compiler::read_instruction(uint8_t command_index, uint8_t command_flag)
{
	nlohmann::json instruction;
	using ink::Command;
	// Determine the number of parameters based on the command type
	const char* command_string = command_index < ( uint8_t ) Command::NUM_COMMANDS
	                               ? CommandStrings[command_index]
	                               : "<UNKNOWN>";
	auto length_of_instructions = _story.end() - _story.instructions();
	switch (static_cast<Command>(command_index)) {
		case Command::STR: {
			auto strng  = read<const char*>();
			instruction = std::string("^") + strng;
		} break;
		case Command::INT: {
			int value   = read<int>();
			instruction = value;
		} break;
		case Command::BOOL: {
			bool value  = read<int>() ? true : false;
			instruction = value;
			break;
		}
		case Command::FLOAT: {
			float value = read<float>();
			instruction = value;
			break;
		}
		case Command::VALUE_POINTER: {
			// TODO: hash lookup
			auto target = read<hash_t>();
			instruction = {
			    {"^var",			                        target},
          {  "ci", static_cast<char>(command_flag) - 1}
			};
			break;
		}
		case Command::DIVERT_VAL: {
			auto target = read<uint32_t>();
			auto target_cmd = *(_story.instructions() + target);
			instruction       = {{"^->", "TODO: emit a named container before this instruction: " + std::string(CommandStrings[target_cmd]) + " @ " + std::to_string(target)}};
			break;
		}
		case Command::LIST: {
			int list_index = read<hash_t>();
			instruction    = "TODO: list " + std::to_string(list_index);
		} break;
		case Command::VOID: {
			instruction = "";
		}; break;
		case Command::TAG: {
			instruction = {
			    {'#', read<const char*>()}
			};
		};
		case Command::DIVERT: {
			auto value   = read<uint32_t>();
			if (value == length_of_instructions) {
				return nullptr;
			}
			instruction = {
			    {"->", get_container_path(value)}
			};
			if (command_flag & ( uint8_t ) CommandFlag::DIVERT_HAS_CONDITION) {
				instruction["c"] = true;
			}
		} break;
		case Command::DIVERT_TO_VARIABLE: {
			auto value   = read<uint32_t>();
			auto str_value = "$r" + std::to_string(value);
			instruction = {
			    {"->", str_value},
					{"var", true},
			};
			if (command_flag & ( uint8_t ) CommandFlag::DIVERT_HAS_CONDITION) {
				instruction["c"] = true;
			}
		} break;
		case Command::TUNNEL: {
			auto value   = read<uint32_t>();
			instruction = {
			    {"->t->", get_container_path(value)}
			};
			if (command_flag & ( uint8_t ) CommandFlag::TUNNEL_TO_VARIABLE) {
				instruction["var"] = true;
			}
		} break;
		case Command::FUNCTION: {
			// f()
			auto value   = read<uint32_t>();
			// TODO: real val
			instruction = {
			    {"f()", value}
			};
			if (command_flag & ( uint8_t ) CommandFlag::FUNCTION_TO_VARIABLE) {
				instruction["var"] = true;
			}
		};
		case Command::DEFINE_TEMP: {
			auto value   = read<uint32_t>();
			auto str_value = "$r" + std::to_string(value);
			instruction = {
			    {"temp=", str_value}
			};
			if (command_flag & ( uint8_t ) CommandFlag::ASSIGNMENT_IS_REDEFINE) {
				instruction["re"] = true;
			}
		} break;
		case Command::SET_VARIABLE: {
			auto value   = read<uint32_t>();
			// TODO: Actual variable name
			instruction = {
			    {"VAR=", value}
			};
			if (command_flag & ( uint8_t ) CommandFlag::ASSIGNMENT_IS_REDEFINE) {
				instruction["re"] = true;
			}
		} break;
		case Command::PUSH_VARIABLE_VALUE: {
			auto value   = read<uint32_t>();
			// TODO: Actual variable name

			instruction = {
			    {"VAR?", value}
			};
		} break;
		case Command::READ_COUNT: {
			auto value   = read<uint32_t>();
			// TODO: Actual variable name
			instruction = {
			    {"CNT?", value}
			};
		} break;


		case Command::CHOICE: {
			auto value   = read<uint32_t>();
			instruction = {
			    {  "*",			                        get_container_path(value)},
          {"flg", static_cast<uint8_t>(command_flag)}
			};

		} break;
		case Command::CALL_EXTERNAL: {
			int numArgs = command_flag;
			auto hash    = read<uint32_t>();
			// pop off the function call
			read<uint8_t>();
			read<uint8_t>();
			auto val     = read<uint32_t>();
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
			// skip the value
			read<uint32_t>();
		}
		default: {
			instruction = command_string;
			printf("Unknown command: %d\n", command_index);
		} break;
	}


	return instruction;
}

std::vector<nlohmann::json> reverse_compiler::decompile_instructions()
{
	std::vector<nlohmann::json> instructions;
	_ptr = _story.instructions();
	while (_ptr < _story.end()) {
		container_t container_id;
		std::vector<container_t> container_ids;
		uint8_t command_index;
		uint8_t command_flag = 0;
		uint32_t indexToReturn = 0;
		auto get_command_flag_if_start_or_end = [&]()
		{
			// peek at the next command
			if (*_ptr == ( uint8_t ) Command::START_CONTAINER_MARKER || *_ptr == ( uint8_t ) Command::END_CONTAINER_MARKER) {
				command_index = read<uint8_t>();
				command_flag = read<uint8_t>();
				indexToReturn = read<uint32_t>();
				return true;
			}
			// anonymous container, consume next instruction
			return false;
		};
		if (is_end_of_container(container_ids)) {
			auto ret = get_command_flag_if_start_or_end();
			for (int i = 0; i < container_ids.size(); i++) {
				end_container(indexToReturn, command_flag);
			}
			if (ret) continue;
		}
		if (is_start_of_container(container_id)) {
			auto ret = get_command_flag_if_start_or_end();
			start_container(container_id, indexToReturn, command_flag);
			if (ret) continue;
		} 
		
		command_index = read<uint8_t>();
		command_flag = read<uint8_t>();
		auto instruction = read_instruction(command_index, command_flag);
		if (instruction != nullptr) {
			_emitter.write_instruction(instruction);
			instructions.push_back(instruction);
		}
	}

	return instructions;
}

std::vector<uint32_t> reverse_compiler::read_divert_offsets()
{
	std::vector<uint32_t> offsets;
	_ptr = _story.instructions();
	auto push_offset = [&](uint32_t target)
	{
		if (std::count(offsets.begin(), offsets.end(), target) == 0) {
			offsets.push_back(target);
		}
	};
	while (_ptr < _story.end()) {
		container_t container_id;
		uint8_t command_index;
		uint8_t command_flag = 0;
		uint32_t indexToReturn = 0;
		command_index = read<uint8_t>();
		command_flag = read<uint8_t>();
		switch((Command)command_index) {
			case Command::DIVERT: {
				auto target = read<uint32_t>();
				push_offset(target);
			} break;
			case Command::CHOICE: {
				auto target = read<uint32_t>();
				push_offset(target);
			} break;
			case Command::DIVERT_VAL: {
				auto target = read<uint32_t>();
		    // TODO: something with this
			} break;
			case Command::START_CONTAINER_MARKER:
			case Command::END_CONTAINER_MARKER: {
				read<uint32_t>(); // skip
			} break;
			// TODO: others
			default: {
				read_instruction(command_index, command_flag);
			} break;
		}
	}
	_ptr = _story.instructions();
	std::sort(offsets.begin(), offsets.end());
	return offsets;
}

void reverse_compiler::start_container(container_t container_id, uint32_t indexToReturn, uint32_t flags)
{
	// get the hash from the container hash table
	auto name = _container_info[container_id].named ? "container-" + std::to_string(container_id) : "";
	container_stack.push_back(container_id);
	_curr_container_id = container_id;
	_curr_container_level = _emitter.start_container(name);
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

std::string reverse_compiler::get_container_path(uint32_t offset)
{
	// TODO: need to do relative/absolute paths
	if (start_offset_container_map.find(offset) != start_offset_container_map.end()) {
		auto id = start_offset_container_map[offset];
		return "container-" + std::to_string(id);
	}
	return "<ANONYMOUS_CONTAINER @ OFFSET " + std::to_string(offset) + ">";
}

reverse_compiler::container_info& reverse_compiler::get_container_info(const container_t container_id)
{
	return _container_info[container_id];
}

bool reverse_compiler::is_start_of_container(container_t& container_id)
{
	if (start_offset_container_map.find(_ptr - _story.instructions()) != start_offset_container_map.end()) {
		container_id = start_offset_container_map[_ptr - _story.instructions()];
		return true;
	}
	return false;
}

bool reverse_compiler::is_end_of_container(std::vector<container_t>& container_id)
{
	if (end_offset_container_map.find(_ptr - _story.instructions()) != end_offset_container_map.end()) {
		container_id = end_offset_container_map[_ptr - _story.instructions()];
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