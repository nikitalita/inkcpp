#include "catch.hpp"

#include <story.h>
#include <globals.h>
#include <runner.h>
#include <compiler.h>
#include <decompiler.h>
#include <fstream>
#include "..//inkcpp_decompiler/binstr_modifier.h"
#include "..//inkcpp_compiler/json.hpp"
using namespace ink::runtime;

static constexpr const char* OUTPUT_PART_1 = "Once upon a time...\n";
static constexpr const char* OUTPUT_PART_2
    = "There were two choices.\nThey lived happily ever after.\n";
static constexpr size_t CHOICE = 0;

SCENARIO("test dumping string table")
{

	// auto compiler = GENERATE("inklecate");
	auto compiler = GENERATE("inklecate", "inky");
	GIVEN(compiler)
	{
		auto input_file  = std::string(INK_TEST_RESOURCE_DIR "simple-1.1.1-") + compiler + ".json";
		auto output_file = std::string(INK_TEST_RESOURCE_DIR "simple-1.1.1-") + compiler + ".bin";
		ink::compiler::run(input_file.c_str(), output_file.c_str());
		// auto   ink    = story::from_file(output_file.c_str());
		// runner thread = ink->new_runner();
		//
		// THEN("Expect normal output")
		// {
		// 	REQUIRE(thread->getall() == OUTPUT_PART_1);
		// 	REQUIRE(thread->has_choices());
		// 	REQUIRE(thread->num_choices() == 2);
		// 	thread->choose(CHOICE);
		// 	REQUIRE(thread->getall() == OUTPUT_PART_2);
		// }
		auto new_output_file
		    = std::string(INK_TEST_RESOURCE_DIR "simple-1.1.1-") + compiler + ".strings.json";
		ink::decompiler::write_string_table_json(output_file.c_str(), new_output_file.c_str());
		// read in the `output_file` into a memstream
		auto in   = std::ifstream(output_file, std::ios::binary | std::ios::ate);
		auto size = in.tellg();
		in.seekg(0, std::ios::beg);
		// read it to a buffer
		std::vector<unsigned char> buffer(size);
		in.read(reinterpret_cast<char*>(buffer.data()), size);

		// create a bidirectional std stream buffer to use as output for the decompiler
		std::string data(size, '\0');
		auto        out = std::ostringstream(data, std::ios_base::out);

		ink::decompiler::binstr_modifier   modifier(buffer.data(), size, false);
		auto                               table = modifier.get_string_table();
		std::map<std::string, std::string> orig_to_replace;
		for (auto& [offset, str] : table) {
			orig_to_replace[str] = str;
		}

		auto writeSize = modifier.modify_string_table(orig_to_replace, out);
		auto dupeOut   = out.str();
		//"Once upon a time..."
		REQUIRE(size == writeSize);
		// compare the output of the decompiler with the original file
		for (size_t i = 0; i < size; i++) {
			REQUIRE(buffer[i] == static_cast<unsigned char>(dupeOut[i]));
		}
		// change "Once upon a time..." to "Unce upon a time..."
		// check if it has it first
		REQUIRE(orig_to_replace.find("Once upon a time...") != orig_to_replace.end());
		orig_to_replace["Once upon a time..."] = "Unce upon a time...";
		out                                    = std::ostringstream(data, std::ios_base::out);
		writeSize                              = modifier.modify_string_table(orig_to_replace, out);
		auto modOut                            = out.str();
		REQUIRE(size == writeSize);
		// header size + 1
		auto diff_byte = static_cast<unsigned char>(modOut[ink::internal::header::Size]);
		REQUIRE(diff_byte == 'U');

		THEN("Woop!") { REQUIRE(true); }
	}
}

SCENARIO("Full test of dumping and modifying string table")
{
	auto test_file = GENERATE(
	    "AHF", "ChoiceBracketStory", "EmptyStringForDivert", "ExternalFunctionsExecuteProperly",
	    "FallBack", "GlobalStory", "LabelConditionStory", "LinesStory", "ListLogicStory", "ListStory",
	    "LookaheadSafe", "MoveTo", "NoEarlyTags", "ObserverStory", "SimpleStoryFlow", "TagsStory",
	    "TheIntercept", "ThirdTierChoiceAfterBracketsStory", "UTF8Story"
	);
	GIVEN(test_file)
	{
		std::string input_file = std::string(INK_TEST_RESOURCE_DIR) + test_file + ".bin";
		std::string output_json_strings
		    = std::string(INK_TEST_RESOURCE_DIR) + test_file + ".strings.json";
		ink::decompiler::write_string_table_json(input_file.c_str(), output_json_strings.c_str());
		// read in the `input_file` into a memstream
		auto in   = std::ifstream(input_file, std::ios::binary | std::ios::ate);
		auto size = in.tellg();
		in.seekg(0, std::ios::beg);
		// read it to a buffer
		std::vector<unsigned char> buffer(size);
		in.read(reinterpret_cast<char*>(buffer.data()), size);

		// read the output json strings
		auto           jsonIn = std::ifstream(output_json_strings);
		// read it into a json object
		nlohmann::json json_table;
		jsonIn >> json_table;
		std::map<std::string, std::string> orig_to_replace;
		for (auto& [key, value] : json_table.items()) {
			orig_to_replace[key] = value.get<std::string>();
		}

		// create a bidirectional std stream buffer to use as output for the decompiler
		std::string data(size, '\0');
		auto        out = std::ostringstream(data, std::ios_base::out);

		ink::decompiler::binstr_modifier modifier(buffer.data(), size, false);
		auto                             writeSize = modifier.modify_string_table(orig_to_replace, out);
		auto                             dupeOut   = out.str();
		REQUIRE(size == writeSize);
		// compare the output of the decompiler with the original file
		for (size_t i = 0; i < size; i++) {
			REQUIRE(buffer[i] == static_cast<unsigned char>(dupeOut[i]));
		}
	}
}