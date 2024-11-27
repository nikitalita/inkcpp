#pragma once

#include "binary_reader.h"
#include "command.h"
#include "json_emitter.h"
#include "compilation_results.h"
#include "story_impl.h"
#include <vector>
#include <string>

namespace ink::compiler::internal
{
class reverse_compiler
{
	std::vector<nlohmann::json> read_instructions();
	nlohmann::json              read_parameter(ink::Command command);

public:
	reverse_compiler(binary_reader& reader, json_emitter& emitter, compilation_results* results);
	void compile();

private:
	binary_reader&       _reader;
	json_emitter&        _emitter;
	compilation_results* _results;

	std::vector<std::string>    _strings; // String table
	std::vector<nlohmann::json> _lists;   // List table
};
} // namespace ink::compiler::internal