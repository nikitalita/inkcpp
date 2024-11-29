#include "reverse_compiler.h"
#include "command.h"

namespace ink::decompiler::internal
{
reverse_compiler::reverse_compiler(ink::runtime::internal::story_impl& story, json_emitter& emitter)
    : _story(story)
    , _emitter(emitter)
{
	_ptr = story.instructions();
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
	offset_t str = read<offset_t>();
	return _story.string(str);
}

void reverse_compiler::decompile()
{
	// At this point, _story has already parsed the header, string table, list definitions, and
	// containers.
	_ptr = _story.instructions();
	// Start the JSON output
	_emitter.start(_story.get_header().ink_version_number);


	// Parse containers
	decompile_containers();

	// Parse list definitions
	decompile_list_definitions();


	// Finish the JSON output
	_emitter.finish();
}

void reverse_compiler::decompile_list_definitions()
{
	// Assuming _story has a method to get list definitions
	const list_flag* lists = _story.lists();
	while (lists->list_id != ~0) {
		_list_meta.new_list(_story.list_meta());
		while (lists->list_id != ~0) {
			_list_meta.new_flag(_story.list_meta(), lists->flag);
			lists++;
		}
		_list_meta.sort();
	}
}

void reverse_compiler::decompile_containers()
{
	const uint32_t* iter = nullptr;
	container_t     index;
	ip_t            offset;
	while (_story.iterate_containers(iter, index, offset)) {
		nlohmann::json container = nlohmann::json::array();
		decompile_container(offset, container);
		_emitter.write_container(container);
	}
}

void reverse_compiler::decompile_container(ip_t& ptr, nlohmann::json& container)
{
	while (ptr < _story.end()) {
		decompile_command(ptr, container);
	}
}

void reverse_compiler::decompile_command(ip_t& ptr, nlohmann::json& container)
{
	Command     cmd  = static_cast<Command>(read<uint8_t>());
	CommandFlag flag = static_cast<CommandFlag>(read<uint8_t>());

	switch (cmd) {
		case Command::START_CONTAINER_MARKER: {
			container_t    index = read<container_t>();
			nlohmann::json sub_container;
			decompile_container(ptr, sub_container);
			container.push_back(sub_container);
		} break;
		case Command::END_CONTAINER_MARKER: return;
		case Command::STR: {
			const char* str = read<const char*>();
			container.push_back(std::string("^") + str);
		} break;
		case Command::DIVERT:
		case Command::DIVERT_TO_VARIABLE:
		case Command::DIVERT_VAL:
		case Command::TUNNEL:
		case Command::DEFINE_TEMP:
		case Command::SET_VARIABLE:
		case Command::VALUE_POINTER:
		case Command::PUSH_VARIABLE_VALUE:
		case Command::CHOICE:
		case Command::READ_COUNT:
		case Command::FUNCTION:
		case Command::CALL_EXTERNAL:
		case Command::LIST: decompile_complex_command(ptr, container); break;
		default: throw std::runtime_error("Unknown command");
	}
}

void reverse_compiler::decompile_complex_command(ip_t& ptr, nlohmann::json& container)
{
	// Implement parsing for complex commands
	// This will involve reading additional data based on the command type
	// and adding the appropriate JSON representation to the container
}

} // namespace ink::decompiler::internal