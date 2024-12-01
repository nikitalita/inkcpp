#pragma once

#include "json.hpp"
#include <ostream>

namespace ink::decompiler::internal
{
class json_emitter
{
public:
	json_emitter(std::ostream& output);
	void start(int ink_version);
	void finish();
	// Start a container
	uint32_t start_container(const std::string& name);

	// End a container
	uint32_t end_container();

	void write_string(const std::string& str);
	void write_list(const std::vector<std::string>& list);
	void write_instruction(const nlohmann::json& instruction);
	void write_container(const nlohmann::json& container);
	void write_listDefs(const nlohmann::json& list);

private:
	nlohmann::json * get_back();
	std::ostream&  _output;
	nlohmann::json _json;
	std::vector<nlohmann::json*> _container_stack;

	// stack
};
} // namespace ink::decompiler::internal