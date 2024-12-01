#include "json_emitter.h"

namespace ink::decompiler::internal
{
json_emitter::json_emitter(std::ostream& output)
		: _output(output)
{
}

void json_emitter::start(int ink_version)
{
	_json["inkVersion"] = ink_version;
	_json["root"]       = nlohmann::json::array();
	_json["listDefs"]   = nlohmann::json::object();
	_container_stack.push_back(&_json["root"]);
}

void json_emitter::finish()
{
	// make _json.dump use the replace exception handler
	_output << _json.dump(4, ' ', false, nlohmann::detail::error_handler_t::replace) << std::endl;
}

void json_emitter::write_string(const std::string& str) { get_back()->push_back(str); }

void json_emitter::write_list(const std::vector<std::string>& list)
{
	get_back()->push_back(list);
}

void json_emitter::write_listDefs(const nlohmann::json& list) { _json["listDefs"] = list; }

nlohmann::json* json_emitter::get_back()
{
	if (_container_stack.empty()) {
		return &_json["root"];
	}
	return _container_stack.back();
}

void json_emitter::write_instruction(const nlohmann::json& instruction)
{
	get_back()->push_back(instruction);
}

void json_emitter::write_container(const nlohmann::json& container)
{
	get_back()->push_back(container);
}

uint32_t json_emitter::start_container(const std::string& name){
	auto container = nlohmann::json::array();
	if (!name.empty()) {
		const nlohmann::json pushb = {{name, container}};
		if (_container_stack.back()->is_object()) {
			(*_container_stack.back())[name] = container;
			_container_stack.push_back(&(*_container_stack.back())[name]);
		} else {
				_container_stack.back()->push_back({{name, container}});
				_container_stack.push_back(&_container_stack.back()->back()[name]);
		}
	} else {
		_container_stack.back()->push_back(container);
		_container_stack.push_back(&_container_stack.back()->back());
	}
	return _container_stack.size() - 1;
}

uint32_t json_emitter::end_container()
{
	if (_container_stack.empty()) {
		printf("WARNING: Empty container stack before pop!\n");
	} else {
		_container_stack.pop_back();
	}
	if (_container_stack.empty()) {
		printf("WARNING: Empty container stack after pop!\n");
		_container_stack.push_back(&_json["root"]);
	}
	return _container_stack.size() - 1;
}
}// namespace ink::decompiler::internal