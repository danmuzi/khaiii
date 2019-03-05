/**
 * @author  Jamie (jamie.lim@kakaocorp.com)
 * @copyright  Copyright (C) 2018-, Kakao Corp. All rights reserved.
 */


#ifndef SRC_MAIN_CPP_KHAIII_MEMMAPFILE_HPP_
#define SRC_MAIN_CPP_KHAIII_MEMMAPFILE_HPP_


//////////////
// includes //
//////////////
#include <fcntl.h>

#if defined(_WIN32) || defined(_WIN64)	// Windows
	#include <io.h>
	#include <Windows.h>
	#include <fileapi.h>
	#include <memoryapi.h>
	#include <handleapi.h>
	#include <sys/stat.h>
#else	// Other OS (Mac OS, Linux)
	#include <sys/mman.h>
	#include <unistd.h>
#endif

#include <fstream>
#include <string>

#include "fmt/format.h"

#include "khaiii/KhaiiiApi.hpp"


namespace khaiii {


/**
 * memory mapped file
 */
template<typename T>
class MemMapFile {
public:
	/**
	 * dtor
	 */
	virtual ~MemMapFile() {
		close();
	}

	std::string ReplaceAll(std::string &str, const std::string& from, const std::string& to) {
		size_t start_pos = 0; //string처음부터 검사
		while ((start_pos = str.find(from, start_pos)) != std::string::npos)  //from을 찾을 수 없을 때까지
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // 중복검사를 피하고 from.length() > to.length()인 경우를 위해서
		}
		return str;
	}

	int getFileSize(char *filename) {
		struct _stati64 statbuf;

		if (_stati64(filename, &statbuf)) return -1; // 파일 정보 얻기: 에러 있으면 -1 을 반환

		return statbuf.st_size;                        // 파일 크기 반환
	}

	/**
	 * open memory mapped file
	 * @param  path  path
	 */
	void open(std::string path) {
		close();

#if defined(_WIN32) || defined(_WIN64)	// Windows
        ReplaceAll(path, std::string("/"), std::string("\\\\"));
        HANDLE _file_read = CreateFile(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (_file_read == INVALID_HANDLE_VALUE) {
			throw Except(fmt::format("fail to open file: {}", path));
		}
		LARGE_INTEGER _file_size;
		GetFileSizeEx(_file_read, &_file_size);
		_byte_len = _file_size.QuadPart;
		HANDLE _file_map = CreateFileMapping(_file_read, NULL, PAGE_READONLY, 0, 0, NULL);
		_data = reinterpret_cast<const T*>((T*)MapViewOfFile(_file_map, FILE_MAP_READ, 0, 0, _byte_len));

        if (_file_map != INVALID_HANDLE_VALUE)
            CloseHandle(_file_map);

        if (_file_read != INVALID_HANDLE_VALUE)
            CloseHandle(_file_read);
#else	// Other OS (Mac OS, Linux)
        int fd = ::open(path.c_str(), O_RDONLY, 0660);
        if (fd == -1) throw Except(fmt::format("fail to open file: {}", path));
        std::ifstream fin(path, std::ifstream::ate | std::ifstream::binary);
        _byte_len = fin.tellg();
        if (_byte_len == -1) throw Except(fmt::format("fail to get size of file: {}", path));
        assert(_byte_len % sizeof(T) == 0);
        _data = reinterpret_cast<const T*>(::mmap(0, _byte_len, PROT_READ, MAP_SHARED, fd, 0));
        ::close(fd);
        if (_data == MAP_FAILED) {
            throw Except(fmt::format("fail to map file to memory: {}", path));
}
#endif
		_path = path;
	}

	/**
	 * close memory mapped file
	 */
	void close() {
		if (_data) {
#if defined(_WIN32) || defined(_WIN64)	// Windows
            UnmapViewOfFile(_data);
#else
            if (::munmap(const_cast<T*>(_data), _byte_len) == -1) {
                throw Except(fmt::format("fail to close memory mapped file: {}", _path));
            }
#endif
		}
		_path = "";
		_data = nullptr;
		_byte_len = -1;
	}

	/**
	 * get pointer of data
	 * @return  start address of data
	 */
	const T* data() const {
		assert(_data != nullptr && _byte_len >= sizeof(T));
		return _data;
	}

	/**
	 * get data size
	 * @return  number of data elements (not byte length)
	 */
	int size() const {
		assert(_data != nullptr && _byte_len >= sizeof(T));
		return _byte_len / sizeof(T);
	}

private:
	std::string _path;    ///< file path
	const T* _data = nullptr;    ///< memory data
	int _byte_len = -1;    ///< byte length
};


}    // namespace khaiii


#endif    // SRC_MAIN_CPP_KHAIII_MEMMAPFILE_HPP_
