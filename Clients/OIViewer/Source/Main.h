#pragma once

#include <LLUtils/StringDefs.h>

LLUtils::native_string_type CompileFilePathFromArguments(int argc, const LLUtils::native_char_type* const* argv);
int RunViewer(const LLUtils::native_string_type& filePath);
