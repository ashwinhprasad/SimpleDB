#pragma once
#include <cstdint>
#include <string>
#include <vector>

constexpr size_t PAGE_SIZE = 8 * 1024; // 8KB

std::vector<uint8_t> read_page(const std::string& file_name, uint64_t page_id);
void write_page(const std::string& file_name, uint64_t page_id, const std::vector<uint8_t>& data);
