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
	void write_string(const std::string& str);
	void write_list(const std::vector<std::string>& list);
	void write_container(const nlohmann::json& container);

private:
	std::ostream&  _output;
	nlohmann::json _json;
};
} // namespace ink::decompiler::internal