#include "decompiler.h"
#include "args.h"
#include <filesystem>
int main(int argc, char* argv[]) {
	// reverse("test.bin", "test.json");
	std::string exe_name = std::filesystem::path(argv[0]).filename().string();
	args::ArgumentParser parser("InkBIN string modifier", "\n\tUsage:\n\t\t"
		+ exe_name + " --dump <INPUT_BIN> <OUTPUT_STRING_TABLE_JSON>\n\t\t"+
		exe_name + " --replace=<INPUT_JSON> <INPUT_BIN> <OUTPUT_BIN>");
	args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
	args::Group group(parser, "Main commands:", args::Group::Validators::Xor);
	args::Flag dump(group, "dump", "Dump string table to json file", {'d', "dump"});
	args::ValueFlag<std::string> replace_with(group, "replace", "Replace string table using json file", {"replace"});
	// Positional argument for bin file
	args::Positional<std::string> bin_file(parser, "binfile", "The binary file to modify");
	args::Positional<std::string> out_file(parser, "outfile", "The output file to write to");
	try
	{
		parser.ParseCLI(argc, argv);
	}
	catch (args::Help)
	{
		std::cout << parser;
		return 0;
	}
	catch (args::ParseError e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}
	catch (args::ValidationError e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}
	if (!bin_file) {
		std::cerr << "No binary file specified" << std::endl;
		std::cerr << parser;
		return 1;
	}
	auto bin_str = bin_file.Get();
	// check for bin_file's existence
	if (!std::filesystem::exists(bin_str)) {
		std::cerr << "File does not exist: " << bin_str << std::endl;
		return 1;
	}
	std::string out_str;
	if (!out_file) {
		if (dump) {
			out_str = bin_str.substr(0, bin_str.find_last_of('.')) + ".strings.json";
		} else if (replace_with) {
			out_str = bin_str.substr(0, bin_str.find_last_of('.')) + ".modified.bin";
		}
	} else {
		out_str = out_file.Get();
	}

	try {
		if (dump) {
			ink::decompiler::write_string_table_json(bin_str.c_str(), out_str.c_str());
			printf("Wrote string table to %s\n", out_str.c_str());
			printf("JSON file is formatted as '\"<original string>\": \"<destination string>\"'\n");
			printf("Modify the destination strings (i.e. the strings after the ':'),\nand then use --replace to replace the string table\n");
		} else if (replace_with) {
			auto input_str = replace_with.Get();
			// check for input_str's existence
			if (!std::filesystem::exists(input_str)) {
				std::cerr << "File does not exist: " << input_str << std::endl;
				return 1;
			}
			ink::decompiler::modify_string_table(bin_str.c_str(), input_str.c_str(), out_str.c_str());
		}
		// just print out whatever exception it is
	} catch (std::exception& e) {
		std::cerr << "FATAL ERROR: ";
		std::cerr << e.what() << std::endl;
		return 1;
	}
}

