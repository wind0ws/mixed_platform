#pragma once
#ifndef __LCU_STRINGS_HEADER
#define __LCU_STRINGS_HEADER

#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#define strcasecmp stricmp
#define strncasecmp strnicmp
//to make MSC happy
#define stricmp _stricmp
#define strnicmp _strnicmp

/**
 * to make MSC happy
 * origin strcpy_s(dest, destSize, source)
 * here we assume that "dest" is big enough to storage "source", so we use 'strlen(source) + 1' as destSize.
 * be careful! 
 */
#define strcpy(dest, source) strcpy_s(dest, strlen(source) + 1, source) 
#define strncpy(dest, source, max_count) strncpy_s(dest, (max_count) + 1, source, max_count)
#define strcat(dest, source) strcat_s(dest, strlen(dest) + strlen(source) + 1, source)
#define STRTOK strtok_s
//#define snprintf(buf, buf_size, format, ...) \
//        _snprintf_s(buf, buf_size, (buf_size) - 1, format, ## __VA_ARGS__)
#else
//in Unix platform. use strtok_r
#define STRTOK strtok_r
//stricmp is windows function.
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif // _WIN32

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(__GLIBC__) || defined(_WIN32)
	/* Declaration of strlcpy() for platforms that don't already have it. */
	size_t strlcpy(char* dst, const char* src, size_t size);
	size_t strlcat(char* dst, const char* src, size_t size);
#endif
	/**
	 * replace the "pattern" to "replacement" from "original".
	 * warning: return string is malloced, need free after use!
	 * @param original: the str to replace
	 * @param pattern: the replace pattern
	 * @param replacement: replace pattern to this
	 */
	char* strreplace(char const* const original,
		char const* const pattern, char const* const replacement);

	/**
	 * split string by delimiter
	 * @param recv_splited_str: the pointer you want to receive spited string.
	 * @param p_splited_nums: how many char * do you provide in rec_splited_str, 
	 *                        this value will rewrite to real split numbers after return.
	 * @param src_str: the origin str that you want to split
	 * @param delimiter: the string of delimiter
	 */
	void strsplit(char* recv_splited_str[], int* p_splited_nums, 
		const char src_str[], const char* delimiter);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // __LCU_STRINGS_HEADER