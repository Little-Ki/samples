#include "zipper.h"
#include <algorithm>
#include <array>
#include <fstream>

namespace zipper {

    static bool is_big_endian() {
        union {
            uint32_t i;
            char c[4];
        } bint = {0x01};
        return bint.c[0] != 1;
    }

    template <typename T>
    void byte_reverse(T &val) {
        auto buf = reinterpret_cast<char *>(&val);
        for (auto i = 0; i < (sizeof(T) >> 1); i++) {
            std::swap(buf[i], buf[sizeof(T) - i - 1]);
        }
    }

    class ifstream_t : public std::ifstream {
    private:
        size_t _size;
        bool _use_reverse;

    public:
        ifstream_t(bool use_reverse = false) : _use_reverse(use_reverse) {};

        bool try_open(const std::string &file_name) {
            open(file_name, std::ios::in | std::ios::binary);

            if (good()) {
                seekg(0, std::ios::end);
                _size = tellg();
                seekg(0, std::ios::beg);
                return true;
            }

            return false;
        }

        std::string read_str(size_t size) {
            std::vector<char> result;
            result.resize(size + 1, '\0');
            std::ifstream::read(result.data(), size);
            return result.data();
        }

        template <typename T>
        void read(T* buffer, size_t count = 1) {
            auto raw = reinterpret_cast<char *>(buffer);
            std::ifstream::read(raw, sizeof(T) * count);
            if (_use_reverse) {
                while(count --) {
                    byte_reverse(buffer[count - 1]);
                }
            }
        }

        ~ifstream_t() {
            if (good())
                close();
        }

        const size_t &size() { return _size; }
    };

    class ofstream_t : public std::ofstream {
    private:
        bool _use_reverse;

    public:
        ofstream_t(bool auto_reverse = false) : _use_reverse(auto_reverse) {};

        bool try_open(const std::string &file_name) {
            open(file_name, std::ios::out | std::ios::binary | std::ios::trunc);
            return good();
        }

        template <typename T>
        void write(T value) {
            auto raw = reinterpret_cast<char *>(&value);
            if (_use_reverse)
                byte_reverse(value);
            std::ofstream::write(raw, sizeof(T));
        }

        template <typename T>
        void write(size_t at, T value) {
            seekg(at, beg);
            ofstream_t::write(value);
        }

        ~ofstream_t() {
            if (good())
                close();
        }
    };

    EndOfCentralDirectoryRecord read_eocd(ifstream_t &in) {
        EndOfCentralDirectoryRecord result;
        in.read(&result.signature);
        in.read(&result.disk_number);
        in.read(&result.directory_disk_number);
        in.read(&result.directory_entries);
        in.read(&result.directory_total_entires);
        in.read(&result.directory_size);
        in.read(&result.directory_offset);
        in.read(&result.comment_length);
        return result;
    }

    CentralDirectoryFileHeader read_cdfh(ifstream_t &in) {
        CentralDirectoryFileHeader result;
        in.read(&result.signature);
        in.read(&result.version_made);
        in.read(&result.version_extract);
        in.read(&result.bitflags);
        in.read(&result.compression_method);
        in.read(&result.modify_time);
        in.read(&result.modify_date);
        in.read(&result.crc32);
        in.read(&result.compressed_size);
        in.read(&result.uncompressed_size);
        in.read(&result.filename_length);
        in.read(&result.extra_length);
        in.read(&result.comment_length);
        in.read(&result.disk_number);
        in.read(&result.internal_attributes);
        in.read(&result.external_attributes);
        in.read(&result.file_offset);
        return result;
    }

    LocalFileHeader read_fh(ifstream_t &in) {
        LocalFileHeader result;
        in.read(&result.signature);
        in.read(&result.version);
        in.read(&result.bitflags);
        in.read(&result.compression_method);
        in.read(&result.modify_time);
        in.read(&result.modify_date);
        in.read(&result.crc32);
        in.read(&result.compressed_size);
        in.read(&result.uncompressed_size);
        in.read(&result.filename_length);
        in.read(&result.extra_length);
        return result;
    }

    size_t find_eocd(ifstream_t &in) {
        size_t pos = in.size() - 22;
        uint32_t signature = 0;
        uint32_t find = SIG_END_CENTRAL_DIRECTORY;

        do {
            in.seekg(pos, in.beg);
            in.read(&signature);
        } while (signature != find && pos-- > 0);

        if (signature == find)
            return pos;

        return -1;
    }

    bool Zipper::has(const std::string &file_name) {
        return _files.find(file_name) != _files.end();
    }

    Error Zipper::load(const std::string &file_name) {
        ifstream_t in(is_big_endian());

        uint32_t directory_count = 0;
        uint32_t file_count = 0;

        if (!in.try_open(file_name)) {
            return Error::FileError;
        }

        if (in.size() < 22) {
            return Error::InvalidSize;
        }

        auto eocd_pos = find_eocd(in);
        if (eocd_pos == static_cast<size_t>(-1)) {
            return Error::InvalidFile;
        }

        in.seekg(eocd_pos, in.beg);
        auto eocd = read_eocd(in);

        if (eocd.comment_length > 0) {
            _comment = in.read_str(eocd.comment_length);
        }

        auto count = eocd.directory_total_entires;

        in.seekg(eocd.directory_offset, in.beg);

        while (count--) {
            ZipFile file;

            size_t pos = in.tellg();
            auto cdfh = read_cdfh(in);

            if (cdfh.filename_length) {
                file.file_name = in.read_str(cdfh.filename_length);
            }

            if (cdfh.extra_length) {
                file.extra_fields.resize(cdfh.extra_length);
                in.read(file.extra_fields.data(), cdfh.extra_length);
            }

            if (cdfh.comment_length) {
                file.comment = in.read_str(cdfh.comment_length);
            }

            size_t next = in.tellg();

            in.seekg(cdfh.file_offset, in.beg);
            auto fh = read_fh(in);

            if (cdfh.compressed_size) {
                file.data.resize(cdfh.compressed_size);
                in.ignore(fh.filename_length + fh.extra_length);
                in.read(file.data.data(), cdfh.compressed_size);
                file_count += 1;
            } else {
                directory_count += 1;
            }

            file.version_made = cdfh.version_made;
            file.version_extract = cdfh.version_extract;
            file.bitflags = cdfh.bitflags;
            file.compression_method = cdfh.compression_method;
            file.compressed_size = cdfh.compressed_size;
            file.uncompressed_size = cdfh.uncompressed_size;
            file.modify_time = cdfh.modify_time;
            file.modify_date = cdfh.modify_date;
            file.crc32 = cdfh.crc32;
            file.internal_attributes = cdfh.internal_attributes;
            file.external_attributes = cdfh.external_attributes;

            _files.emplace(file.file_name, file);

            in.seekg(next, in.beg);
        }

        _directory_count = directory_count;
        _file_count = file_count;

        return Error::Success;
    }

    Error Zipper::save(const std::string &file_name) {
        return Error::Success;
    }

    const std::string &Zipper::comment() {
        return _comment;
    }

    const auto &Zipper::file_count() {
        return _file_count;
    }

    const auto &Zipper::directory_count() {
        return _directory_count;
    }

    void Zipper::set_comment(const std::string& comment) {
        _comment = comment;
    }
}