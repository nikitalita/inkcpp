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


std::map<std::string, std::string> get_map_from_json(const std::string &output_json_strings)
{
	auto           jsonIn = std::ifstream(output_json_strings);
	// read it into a json object
	nlohmann::json json_table;
	jsonIn >> json_table;
	std::map<std::string, std::string> orig_to_replace;
	for (auto& [key, value] : json_table.items()) {
		orig_to_replace[key] = value.get<std::string>();
	}
	return orig_to_replace;
}

std::map<std::string, std::string> get_map_from_string_table(const std::map<uint32_t, std::string> & table)
{
	std::map<std::string, std::string> orig_to_replace;
	for (auto& [offset, str] : table) {
		orig_to_replace[str] = str;
	}
	return orig_to_replace;
}

size_t load_to_buffer(const std::string & input_file,std::vector<unsigned char> &buffer )
{
	// read in the `input_file` into a memstream
	auto in   = std::ifstream(input_file, std::ios::binary | std::ios::ate);
	auto size = in.tellg();
	in.seekg(0, std::ios::beg);
	// read it to a buffer
	buffer.resize(size);
	in.read(reinterpret_cast<char*>(buffer.data()), size);
	return size;
}

std::string stripped_string(const std::string &str)
{
	std::string new_string = str;
	new_string.erase(std::find_if(new_string.rbegin(), new_string.rend(), [](unsigned char ch) {
		return ch != '\n';
	}).base(), new_string.end());
	return new_string;
}

void test_unchanged_string_table(const std::string &input_file, const std::string &output_json_strings, const std::string &output_bin)
{
	std::vector<unsigned char> buffer;
	auto size = load_to_buffer(input_file, buffer);
	ink::decompiler::binstr_modifier modifier(buffer.data(), size, false);
	THEN("Expect string dumping to work")
	{
		ink::decompiler::write_string_table_json(input_file.c_str(), output_json_strings.c_str());
		std::map<std::string, std::string> orig_to_replace = get_map_from_json(output_json_strings);
		auto table = modifier.get_string_table();
		for (auto& [offset, str] : table) {
			REQUIRE(orig_to_replace.find(str) != orig_to_replace.end());
			REQUIRE(orig_to_replace[str] == str);
		}
	}
	THEN("Test modifying with an unchanged string table")
	{
		std::map<std::string, std::string> orig_to_replace = get_map_from_string_table(modifier.get_string_table());

		// create a bidirectional std stream buffer to use as output for the decompiler
		auto        out = std::ostringstream(std::string(size, '\0'), std::ios_base::binary | std::ios_base::out);

		auto                             writeSize = modifier.modify_string_table(orig_to_replace, out);
		auto                             dupeOut   = out.str();
		REQUIRE(size == writeSize);
		// compare the output of the decompiler with the original file
		for (size_t i = 0; i < size; i++) {
			REQUIRE(buffer[i] == static_cast<unsigned char>(dupeOut[i]));
		}
		// write it to a file
		std::ofstream out_file(output_bin, std::ios::binary);
		out_file.write(dupeOut.c_str(), dupeOut.size());
	}
}
#define INK_TEST_RESOURCE_OUTPUT_DIR INK_TEST_RESOURCE_DIR "output/"

SCENARIO("Test running bin with modified string table")
{
	// ensure INK_TEST_RESOURCE_OUTPUT_DIR
	if (!std::filesystem::exists(INK_TEST_RESOURCE_OUTPUT_DIR)) {
		std::filesystem::create_directory(INK_TEST_RESOURCE_OUTPUT_DIR);
	}

	auto compiler = GENERATE("inklecate");
	// auto compiler = GENERATE("inklecate", "inky");
	GIVEN(compiler)
	{
		auto input_file  = std::string(INK_TEST_RESOURCE_DIR "simple-1.1.1-") + compiler + ".json";
		auto output_file = std::string(INK_TEST_RESOURCE_OUTPUT_DIR "simple-1.1.1-") + compiler + ".bin";
		ink::compiler::run(input_file.c_str(), output_file.c_str());
		// THEN("test printing the commands")
		// {
		// 	std::vector<unsigned char> buffer;
		// 	auto size = load_to_buffer(output_file, buffer);
		// 	ink::decompiler::binstr_modifier modifier(buffer.data(), size, false);
		// 	std::cout << "*** Original Instructions" << std::endl;
		// 	modifier.print_instructions();
		// 	std::cout << "*** END Original Instructions" << std::endl;
		//
		// }
		THEN("Expect normal output from compiled file")
		{
			auto   ink    = story::from_file(output_file.c_str());
			runner thread = ink->new_runner();
			REQUIRE(thread->getall() == OUTPUT_PART_1);
			REQUIRE(thread->has_choices());
			REQUIRE(thread->num_choices() == 2);
			thread->choose(CHOICE);
			REQUIRE(thread->getall() == OUTPUT_PART_2);
		}
		auto output_json_strings = std::string(INK_TEST_RESOURCE_OUTPUT_DIR "simple-1.1.1-") + compiler + ".strings.json";
		auto output_modified_bin = std::string(INK_TEST_RESOURCE_OUTPUT_DIR "simple-1.1.1-") + compiler + ".modified.bin";
		test_unchanged_string_table(output_file, output_json_strings, output_modified_bin);
		THEN("Expect normal output from output with unchanged string table")
		{
			auto   ink    = story::from_file(output_modified_bin.c_str());
			runner thread = ink->new_runner();
			REQUIRE(thread->getall() == OUTPUT_PART_1);
			REQUIRE(thread->has_choices());
			REQUIRE(thread->num_choices() == 2);
			thread->choose(CHOICE);
			REQUIRE(thread->getall() == OUTPUT_PART_2);
		}

		std::vector<unsigned char> buffer;
		auto size = load_to_buffer(output_file, buffer);
		ink::decompiler::binstr_modifier modifier(buffer.data(), size, false);
		auto orig_to_replace = get_map_from_string_table(modifier.get_string_table());
		// change "Once upon a time..." to "Unce upon a time..."
		std::string stripped_OUTPUT_PART_1 = stripped_string(OUTPUT_PART_1);;
		std::string modded_output_part_1 = OUTPUT_PART_1;
		modded_output_part_1[0] = 'U';
		std::string modded_output_part_1_stripped = stripped_string(modded_output_part_1);
		
		REQUIRE(orig_to_replace.find(stripped_OUTPUT_PART_1) != orig_to_replace.end());
		orig_to_replace[stripped_OUTPUT_PART_1] = modded_output_part_1_stripped;
		THEN("Expect modified output from bin with modified string table")
		{
			auto        out = std::ostringstream(std::string(size, '\0'), std::ios_base::binary | std::ios_base::out);
			auto writeSize= modifier.modify_string_table(orig_to_replace, out);
			auto modOut                            = out.str();
			REQUIRE(size == writeSize);
			// header size + 1
			auto diff_byte = static_cast<unsigned char>(modOut[ink::internal::header::Size]);
			REQUIRE(diff_byte == 'U');

			auto   ink    = story::from_binary((unsigned char*)modOut.c_str(), modOut.size(), false);
			runner thread = ink->new_runner();
			REQUIRE(thread->getall() == modded_output_part_1);
			REQUIRE(thread->has_choices());
			REQUIRE(thread->num_choices() == 2);
			thread->choose(CHOICE);
			REQUIRE(thread->getall() == OUTPUT_PART_2);
		}
		modded_output_part_1 = "************************************************************************************************************HALDO!!!!!!!";
		orig_to_replace[stripped_OUTPUT_PART_1] = modded_output_part_1;
		THEN("Expect modified output from bin modified with strings of different size")
		{
			auto        out = std::ostringstream(std::string(size, '\0'), std::ios_base::binary | std::ios_base::out);
			auto writeSize= modifier.modify_string_table(orig_to_replace, out);
			auto modOut                            = out.str();
			modOut.resize(writeSize);

			auto new_modifier = ink::decompiler::binstr_modifier((unsigned char*)modOut.c_str(), modOut.size(), false);
			// std::cout << "\n\n************************************\n" << "*** MODIFIED Instructions" << std::endl;
			//
			// new_modifier.print_instructions();
			// std::cout << "*** END MODIFIED Instructions" << std::endl << "************************************" << std::endl << std::endl;

			auto   ink    = story::from_binary((unsigned char*)modOut.c_str(), modOut.size(), false);
			runner thread = ink->new_runner();
			REQUIRE(stripped_string(thread->getall()) == modded_output_part_1);
			REQUIRE(thread->has_choices());
			REQUIRE(thread->num_choices() == 2);
			thread->choose(CHOICE);
			REQUIRE(thread->getall() == OUTPUT_PART_2);
		}

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
	// ensure INK_TEST_RESOURCE_OUTPUT_DIR
	if (!std::filesystem::exists(INK_TEST_RESOURCE_OUTPUT_DIR)) {
		std::filesystem::create_directory(INK_TEST_RESOURCE_OUTPUT_DIR);
	}
	GIVEN(test_file)
	{
		std::string input_file = std::string(INK_TEST_RESOURCE_DIR) + test_file + ".bin";
		std::string output_json_strings
		    = std::string(INK_TEST_RESOURCE_OUTPUT_DIR) + test_file + ".strings.json";
		std::string outputbin
				= std::string(INK_TEST_RESOURCE_OUTPUT_DIR) + test_file + ".modified.bin";
		test_unchanged_string_table(input_file, output_json_strings, outputbin);
	}
}

SCENARIO("Test bin version 0")
{
	// ensure INK_TEST_RESOURCE_OUTPUT_DIR
	if (!std::filesystem::exists(INK_TEST_RESOURCE_OUTPUT_DIR)) {
		std::filesystem::create_directory(INK_TEST_RESOURCE_OUTPUT_DIR);
	}

	auto test_file = "$";
	std::string input_file = std::string(INK_TEST_RESOURCE_DIR) + test_file + ".inkb";
	std::string output_json_strings = std::string(INK_TEST_RESOURCE_OUTPUT_DIR) + test_file + ".strings.json";
	std::string output_modded_bin = std::string(INK_TEST_RESOURCE_OUTPUT_DIR) + test_file + ".modified.inkb";
	test_unchanged_string_table(input_file, output_json_strings, output_modded_bin);

}