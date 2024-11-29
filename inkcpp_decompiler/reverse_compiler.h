#pragma once

#include "story_impl.h"
#include "command.h"
#include "json_emitter.h"
#include <vector>
#include "list_data.h"

namespace ink::decompiler::internal
{
class reverse_compiler
{
public:
	reverse_compiler(ink::runtime::internal::story_impl& story, json_emitter& emitter);
	void decompile();

private:
	template<typename T>
	inline T    read();
	ip_t        _ptr;
	container_t _next_container_index;

	ink::compiler::internal::list_data _list_meta;
	int                                _ink_version;

	ink::runtime::internal::story_impl& _story;
	json_emitter&                       _emitter;

	void decompile_list_definitions();
	void decompile_containers();
	void decompile_container(ip_t& ptr, nlohmann::json& container);
	void decompile_command(ip_t& ptr, nlohmann::json& container);
	void decompile_complex_command(ip_t& ptr, nlohmann::json& container);
};
} // namespace ink::decompiler::internal