#pragma once

#include "story_impl.h"
#include "command.h"
#include "json_emitter.h"
#include <vector>
#include "list_data.h"
#include "instruction_reader.h"
namespace ink::decompiler::internal
{
class reverse_compiler
{
public:
	reverse_compiler(ink::runtime::internal::story_impl& story, json_emitter& emitter);
	void decompile();

	struct container_info
	{
		container_t id;
		uint32_t startOffset;
		uint32_t endOffset;
		bool named = true;
		bool real = true;
	};
private:
	uint32_t _curr_container_level = 0;
	container_t _curr_container_id = 0;
	std::vector<container_t> container_stack;

	
	ink::compiler::internal::list_data _list_meta;
	int                                _ink_version;
	std::vector<container_info>				_container_info;
	std::map<uint32_t, instruction_info> _instruction_info;
	std::map<uint32_t, uint32_t> start_offset_container_map;
	std::map<uint32_t, std::vector<uint32_t>> end_offset_container_map;
	std::map<hash_t, uint32_t>        _hash_to_offset_map;
	std::map<hash_t, container_t>        _hash_to_id_map;
	ink::runtime::internal::story_impl& _story;
	json_emitter&                             _emitter;
	nlohmann::json                      serialize_instruction(const instruction_info& instruction) const;

	nlohmann::json                      decompile_instruction(uint8_t command_index, uint8_t command_flag);
	std::vector<nlohmann::json>         decompile_instructions();
	std::vector<uint32_t> read_divert_offsets() const;
  void start_container(container_t container_id, uint32_t indexToReturn, uint32_t flags);
	void end_container(uint32_t indexToReturn, uint32_t flags);
	void pop_container_info();

	std::string get_container_path(const uint32_t offset) const;

	const container_info& get_container_info(const container_t container_id) const;

	container_info& get_container_info(container_t container_id);

	bool is_start_of_container(ip_t pos, container_t& container_id) const;
	bool is_end_of_container(ip_t pos, std::vector<container_t>& container_id) const;

	void decompile_list_definitions();
	void decompile_containers();
	void decompile_container(ip_t& ptr, nlohmann::json& container);
	void decompile_command(ip_t& ptr, nlohmann::json& container);
	void decompile_complex_command(ip_t& ptr, nlohmann::json& container);
};
} // namespace ink::decompiler::internal