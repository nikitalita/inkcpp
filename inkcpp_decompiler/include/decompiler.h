/* Copyright (c) 2024 Julian Benda
 *
 * This file is part of inkCPP which is released under MIT license.
 * See file LICENSE.txt or go to
 * https://github.com/JBenda/inkcpp for full license details.
 */
#pragma once

#include <iostream>

namespace ink
{
/** collection of functions to decompile a story.bin to story.json */
namespace decompiler
{
	void reverse(const char* filenameIn, const char* filenameOut);
	void reverse(std::istream& in, std::ostream& out);

	void write_string_table_json(const char *binfileIn, const char *jsonfileOut);
	void write_string_table_json(std::istream& binIn, std::ostream& jsonOut);

	void modify_string_table(const char* binfileIn, const char* jsonfileIn, const char* binfileOut);
	void modify_string_table(std::istream& binIn, std::istream& jsonIn, std::ostream& binOut);
} // namespace decompiler
} // namespace ink
