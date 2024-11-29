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
} // namespace decompiler
} // namespace ink
