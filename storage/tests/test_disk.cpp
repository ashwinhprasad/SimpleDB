#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>
#include <cstring>
#include "storage/disk.hpp"

struct Student {
    std::string name;
    uint8_t age;
    std::string email;
    float height;
    bool operator==(const Student& other) const {
        return name == other.name &&
               age == other.age &&
               email == other.email &&
               height == other.height;
    }
};

std::vector<uint8_t> serialize_student(const Student& s) {
    std::vector<uint8_t> buf;
    auto write_u32 = [&](uint32_t v) {
        for (int i = 0; i < 4; i++) buf.push_back(static_cast<uint8_t>(v >> (i * 8)));
    };
    write_u32(static_cast<uint32_t>(s.name.size()));
    buf.insert(buf.end(), s.name.begin(), s.name.end());
    buf.push_back(s.age);
    write_u32(static_cast<uint32_t>(s.email.size()));
    buf.insert(buf.end(), s.email.begin(), s.email.end());
    uint32_t h;
    std::memcpy(&h, &s.height, sizeof(float));
    for (int i = 0; i < 4; i++) buf.push_back(static_cast<uint8_t>(h >> (i * 8)));
    return buf;
}

Student deserialize_student(const std::vector<uint8_t>& buf) {
    size_t offset = 0;
    auto read_u32 = [&](size_t& off) {
        uint32_t v = 0;
        for (int i = 0; i < 4; i++) v |= (uint32_t(buf[off + i]) << (i * 8));
        off += 4;
        return v;
    };
    uint32_t name_len = read_u32(offset);
    std::string name(reinterpret_cast<const char*>(&buf[offset]), name_len);
    offset += name_len;
    uint8_t age = buf[offset++];
    uint32_t email_len = read_u32(offset);
    std::string email(reinterpret_cast<const char*>(&buf[offset]), email_len);
    offset += email_len;
    uint32_t h = 0;
    for (int i = 0; i < 4; i++) h |= (uint32_t(buf[offset + i]) << (i * 8));
    float height;
    std::memcpy(&height, &h, sizeof(float));
    offset += 4;
    return Student{name, age, email, height};
}


class DiskTest : public ::testing::Test {
protected:
    std::string temp_file(const std::string& name) {
        auto tmp = std::filesystem::temp_directory_path();
        auto pid = std::to_string(::getpid());
        std::string filename = "diskmgr_" + name + "_" + pid + ".bin";
        auto path = tmp / filename;
        std::filesystem::remove(path); // clean before
        return path.string();
    }
};


TEST_F(DiskTest, AllocateAndReadWrite) {
    auto path = temp_file("alloc_rw");
    auto page = read_page(path, 0);
    EXPECT_EQ(page.size(), PAGE_SIZE);
    EXPECT_TRUE(std::all_of(page.begin(), page.end(), [](uint8_t b){ return b == 0; }));
    std::vector<uint8_t> data(PAGE_SIZE, 0);
    data[0] = 1; data[1] = 2; data[2] = 3; data[3] = 4;
    write_page(path, 0, data);
    auto page2 = read_page(path, 0);
    EXPECT_EQ(page2[0], 1);
    EXPECT_EQ(page2[1], 2);
    EXPECT_EQ(page2[2], 3);
    EXPECT_EQ(page2[3], 4);
    auto page_far = read_page(path, 10);
    EXPECT_EQ(page_far.size(), PAGE_SIZE);
    EXPECT_TRUE(std::all_of(page_far.begin(), page_far.end(), [](uint8_t b){ return b == 0; }));
    std::filesystem::remove(path);
}

TEST_F(DiskTest, InvalidLengthWrite) {
    auto path = temp_file("invalid_length");
    std::vector<uint8_t> bad_data = {1, 2, 3};
    EXPECT_THROW(write_page(path, 0, bad_data), std::invalid_argument);
}

TEST_F(DiskTest, SingleStudentReadWrite) {
    auto path = temp_file("student1");
    Student s{"Alice", 20, "alice@example.com", 5.4f};
    std::vector<uint8_t> data(PAGE_SIZE, 0);
    auto serialized = serialize_student(s);
    std::copy(serialized.begin(), serialized.end(), data.begin());
    write_page(path, 0, data);
    auto page = read_page(path, 0);
    Student s2 = deserialize_student(page);
    EXPECT_EQ(s, s2);
    std::filesystem::remove(path);
}

TEST_F(DiskTest, StudentWithLongNameEmail) {
    auto path = temp_file("student2");
    std::string long_name(1000, 'A');
    std::string long_email(2000, 'b');
    long_email += "@example.com";
    Student s{long_name, 30, long_email, 6.1f};
    auto serialized = serialize_student(s);
    ASSERT_LT(serialized.size(), PAGE_SIZE);
    std::vector<uint8_t> data(PAGE_SIZE, 0);
    std::copy(serialized.begin(), serialized.end(), data.begin());
    write_page(path, 0, data);
    auto page = read_page(path, 0);
    Student s2 = deserialize_student(page);
    EXPECT_EQ(s, s2);
    std::filesystem::remove(path);
}
