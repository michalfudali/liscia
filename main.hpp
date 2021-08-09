#pragma once
#include <string>
#include <vector>

std::vector<uint8_t>ReadBytesFromFile(std::string filename, int n);
void WriteBytesToFile(std::string filename, std::vector<uint8_t> bytes);

std::vector<uint8_t> ReadCharactersFromFile(std::string filename, int n);
void WriteCharactersToFile(std::string filename, std::vector<uint8_t> bytes);