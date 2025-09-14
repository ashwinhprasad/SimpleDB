#include "storage/disk.hpp"
#include <fstream>
#include <stdexcept>
#include <stdint.h>
#include <vector>

std::vector<uint8_t> read_page(const std::string& file_name, uint64_t page_id) {
    uint64_t offset = page_id * PAGE_SIZE;
    std::ifstream file(file_name, std::ios::binary);
    if (!file.is_open()) {
        return std::vector<uint8_t>(PAGE_SIZE, 0);
    }
    file.seekg(offset, std::ios::beg);
    std::vector<uint8_t> buf(PAGE_SIZE, 0);
    file.read(reinterpret_cast<char*>(buf.data()), PAGE_SIZE);
    return buf;
}

void write_page(const std::string& file_name, uint64_t page_id, const std::vector<uint8_t>& data) {
    if (data.size() != PAGE_SIZE) {
        throw std::invalid_argument("Page must be exactly PAGE_SIZE bytes");
    }
    uint64_t offset = page_id * PAGE_SIZE;
    std::fstream file(file_name, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        file.open(file_name, std::ios::out | std::ios::binary);
        file.close();
        file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
    }
    file.seekp(offset, std::ios::beg);
    file.write(reinterpret_cast<const char*>(data.data()), PAGE_SIZE);
    file.flush();
}
