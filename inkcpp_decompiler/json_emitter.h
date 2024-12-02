#pragma once

#include <ostream>
#include "json.hpp"

namespace ink::decompiler::internal
{
class json_emitter
{
public:
	json_emitter(std::ostream& output);
	void start(int ink_version);
	void finish();
	// Start a container
	uint32_t start_container(const std::string& name, const std::string& comment = "");

	// End a container
	uint32_t end_container();

	void write_instruction(nlohmann::json& instruction, const std::string& comment = "");
	void write_listDefs(const nlohmann::json& list);

private:
	nlohmann::json * get_back_container();
	std::ostream&  _output;
	nlohmann::json _json;
	std::vector<nlohmann::json*> _container_stack;

	// stack
};
} // namespace ink::decompiler::internal