#include "json_emitter.h"

namespace ink::compiler::internal
{
json_emitter::json_emitter(std::ostream& output)
    : _output(output)
{
}

void json_emitter::start(int ink_version)
{
	_json["inkVersion"] = ink_version;
	_json["root"]       = nlohmann::json::array();
}

void json_emitter::finish()
{
	// make _json.dump use the replace exception handler
	_output << _json.dump(4, ' ', false, nlohmann::detail::error_handler_t::replace) << std::endl;
}

void json_emitter::write_string(const std::string& str) { _json["root"].push_back(str); }

void json_emitter::write_list(const std::vector<std::string>& list)
{
	_json["root"].push_back(list);
}

void json_emitter::write_container(const nlohmann::json& container)
{
	_json["root"].push_back(container);
}
} // namespace ink::compiler::internal