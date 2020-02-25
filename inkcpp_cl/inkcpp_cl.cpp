// inkcpp_cl.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <regex>
#include <cstdlib>

#include <story.h>
#include <runner.h>
#include <compiler.h>
#include <choice.h>
#include <globals.h>

void usage()
{
	using namespace std;
	cout
		<< "Usage: inkcpp_cl <options> <json file>\n"
		<< "\t-o <filename>:\tOutput file name\n"
		<< "\t-p:\tPlay mode\n"
		<< endl;
}

int main(int argc, const char** argv)
{
	// Usage
	if (argc == 1)
	{
		usage();
		return 1;
	}

	// Parse options
	std::string outputFilename;
	bool playMode = false;
	for (int i = 1; i < argc - 1; i++)
	{
		std::string option = argv[i];
		if (option == "-o")
		{
			outputFilename = argv[i + 1];
			i += 1;
		}
		else if (option == "-p")
			playMode = true;
		else
		{
			std::cerr << "Unrecognized option '" << option << "'\n";
		}
	}

	// Get input filename
	std::string inputFilename = argv[argc - 1];

	// If output filename not specified, use input filename as guideline
	if (outputFilename.empty())
	{
		outputFilename = std::regex_replace(inputFilename, std::regex("\\.[^\\.]+$"), ".bin");
	}

	// If input filename is an .ink file
	int val = inputFilename.find(".ink");
	if (val == inputFilename.length() - 4)
	{
		// Create temporary filename
		std::string jsonFile = std::regex_replace(inputFilename, std::regex("\\.[^\\.]+$"), ".tmp");

		// Then we need to do a compilation with inklecate
		std::string command = "inklecate -o " + jsonFile + " " + inputFilename;
		std::system(command.c_str());

		// New input is the json file
		inputFilename = jsonFile;
	}

	// Open file and compile
	{
		ink::compiler::compilation_results results;
		std::ofstream fout(outputFilename, std::ios::binary | std::ios::out);
		ink::compiler::run(inputFilename.c_str(), fout, &results);
		fout.close();

		// Report errors
		for (auto& warn : results.warnings)
			std::cerr << "WARNING: " << warn << '\n';
		for (auto& err : results.errors)
			std::cerr << "ERROR: " << err << '\n';

		if (results.errors.size() > 0 && playMode)
		{
			std::cerr << "Cancelling play mode. Errors detected in compilation" << std::endl;
			return -1;
		}
	}

	if (!playMode)
		return 0;

	// Run the story
	{
		using namespace ink::runtime;

		// Load story
		story* myInk = story::from_file(outputFilename.c_str());

		// Start runner
		runner thread = myInk->new_runner();

		while (true)
		{
			while (thread->can_continue())
				std::cout << thread->getline();

			if (thread->has_choices())
			{
				for (const ink::runtime::choice& c : *thread)
				{
					std::cout << "* " << c.text() << std::endl;
				}

				int c = 0;
				std::cin >> c;
				thread->choose(c);
				continue;
			}

			// out of content
			break;
		}

		return 0;
	}
}