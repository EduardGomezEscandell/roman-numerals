#pragma once

#include "result.h"

// Parses a roman number from the string
// The output is wrapped around a result, and can only be trusted if result error is NULL
struct result parse_roman_number(char const* str);