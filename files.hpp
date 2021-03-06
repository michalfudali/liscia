#pragma once

#include <vector>
#include <cstdint>
#include <string>

std::vector<uint8_t>ReadBytesFromFile(std::string filename, int n);
void WriteBytesToFile(std::string filename, std::vector<uint8_t> bytes);

std::vector<uint8_t> ReadNetASCIIFromFile(std::string filename, int n);
void WriteNetASCIIToFile(std::string filename, std::vector<uint8_t> bytes);