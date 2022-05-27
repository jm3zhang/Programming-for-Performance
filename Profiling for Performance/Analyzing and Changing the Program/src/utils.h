#pragma once

#include "Container.h"
#include <string>

Container<std::string> readFile(std::string fileName);

Container<std::string> readFileLines(std::string fileName, int startLineNumber, int endLineNumber);

std::string bytesToString(uint8_t* bytes, int len);

std::uint8_t* sha256(const std::string str);

std::uint8_t* initChecksum();

std::uint8_t* xorChecksum(std::uint8_t* baseLayer, std::uint8_t* newLayer);
