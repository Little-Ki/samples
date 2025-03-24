#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace zipper {

    constexpr uint32_t SIG_CENTRAL_DIRECTORY = 0x02014b50;
    constexpr uint32_t SIG_END_CENTRAL_DIRECTORY = 0x06054b50;
    constexpr uint32_t SIG_LOCAL_FILE_HEADER = 0x04034b50;
    constexpr uint32_t SIG_DATA_DESCRIPTOR = 0x08074b50;

    constexpr uint16_t METHOD_STORE = 0;
    constexpr uint16_t METHOD_DEFLATE = 0x08;

    constexpr uint16_t VERSION_STORE = 0x000A;
    constexpr uint16_t VERSION_DEFLATE = 0x0014;
    constexpr uint16_t VERSION_ZIP64 = 0x002D;

    constexpr uint16_t DATE_NORMAL = 0x21;
    constexpr uint16_t TIME_NORMAL = 0x00;

    enum class Error {
        Success,
        FileError,
        InvalidFile,
        InvalidSize,
        InvalidSignature,
        UnsupportMethod
    };

    union Date {
        struct {
            int8_t day_of_month : 5;
            int8_t month : 4;
            int8_t offset_1980 : 7;
        };
        uint16_t value;
    };

    union Time {
        struct {
            int8_t half_second : 5;
            int8_t minute : 6;
            int8_t hour : 5;
        };
        uint16_t value;
    };

    union GeneralPurposeBitFlags {
        struct {
            bool encrypted : 1;
            uint8_t compression_options : 2;
            bool crc_and_sizes_in_cd_and_data_descriptor : 1;
            bool enhanced_deflating : 1;
            bool patched_data : 1;
            bool strong_encryption : 1;
            uint8_t unused : 4;
            bool utf8 : 1;
            bool reserved_0 : 1;
            bool central_directory_encrypted : 1;
            bool reserved_1 : 2;
        };
        uint16_t value;
    };

    struct LocalFileHeader {
        uint32_t signature;
        uint16_t version;
        uint16_t bitflags;
        uint16_t compression_method;
        uint16_t modify_time;
        uint16_t modify_date;
        uint32_t crc32;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint16_t filename_length;
        uint16_t extra_length;
    };

    struct CentralDirectoryFileHeader {
        uint32_t signature;
        uint16_t version_made;
        uint16_t version_extract;
        uint16_t bitflags;
        uint16_t compression_method;
        uint16_t modify_time;
        uint16_t modify_date;
        uint32_t crc32;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint16_t filename_length;
        uint16_t extra_length;
        uint16_t comment_length;
        uint16_t disk_number;
        uint16_t internal_attributes;
        uint32_t external_attributes;
        uint32_t file_offset;
    };

    struct EndOfCentralDirectoryRecord {
        uint32_t signature;
        uint16_t disk_number;
        uint16_t directory_disk_number;
        uint16_t directory_entries;
        uint16_t directory_total_entires;
        uint32_t directory_size;
        uint32_t directory_offset;
        uint16_t comment_length;
    };

    struct LocalFileDescriptor {
        uint32_t signature;
        uint32_t crc32;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
    };

    struct ZipFile {
        uint16_t version_made;
        uint16_t version_extract;
        uint16_t bitflags;
        uint16_t compression_method;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint16_t modify_time;
        uint16_t modify_date;
        uint32_t crc32;
        uint16_t internal_attributes;
        uint32_t external_attributes;

        std::vector<uint8_t> data;
        std::vector<uint8_t> extra_fields;
        std::string file_name;
        std::string comment;
    };

    class Zipper {
    public:
        bool has(const std::string &file_name);

        Error load(const std::string &file_name);

        Error save(const std::string &file_name);

        void add(const std::string &file_name, const std::vector<uint8_t>& data);
        
        void add(const std::string &file_name, const std::string& str);
        
        void remove(const std::string &file_name);

        void set_comment(const std::string &comment);

        const std::string &comment();

        const auto &directory_count();

        const auto &file_count();

    private:
        std::string _comment;
        std::unordered_map<std::string, ZipFile> _files;

        uint32_t _directory_count;
        uint32_t _file_count;
    };
}