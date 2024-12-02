#include "decompiler.h"
#include "args.h"
int main(int argc, char* argv[]) {
	// reverse("test.bin", "test.json");
	args::ArgumentParser parser("InkBIN string modifier", "This goes after the options.");
	args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
	args::Group group(parser, "Main commands:", args::Group::Validators::Xor);
	args::Flag dump(group, "dump", "Dump string table to json file", {'d', "dump"});
	args::ValueFlag<std::string> replace_with(group, "replace-with", "Replace string table using json file", {"replace-with"});
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

	if (dump) {
		ink::decompiler::write_string_table_json(bin_str.c_str(), out_str.c_str());
	} else if (replace_with) {
		auto input_str = replace_with.Get();
		ink::decompiler::modify_string_table(bin_str.c_str(), input_str.c_str(), out_str.c_str());
	}
}

