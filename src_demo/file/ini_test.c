#include "mem/mem_debug.h"
#include "file/ini_reader.h"
#include "file/ini_parser.h"
#include "mem/strings.h"
#include "common_macro.h"

#define LOG_TAG "INI_TEST"
#include "log/logger.h"

static const char* test_ini_str = "\
[config]\r\n\
#this is comment\r\n\
#number=1\r\n\
nNum1 = 6\r\n\
test=\r\n\
#中文注释以及没有return符\n\
nNum2 = 2\n\
#test double number\r\n\
nNum3=0.035\r\n\
# 测试16进制数字解析\r\n\
nNum4 = 0xFF\n\
nNUm5 = 0Xff\n\
\r\n\
[config2]\r\n\
#test true false\r\n\
auto_start = FALSE\r\n\
enable_state=true \r\n\
number_bool_state = 0 \r\n\
\r\n\
;test semicolon comment\r\n\
[config3]\r\n\
path= /sdcard/Android/data/  \r\n\
\r\n\
[config4]\r\n\
#test empty value \r\n\
run_mode =  \r\n\
\r\n";

/**
 * ini parse callback
 *   return true continue,
 *   return false will cause 'ini_reader_parse' returned non-zero code.
 */
static int my_ini_reader_handler(void* user,
	const char* section, const char* key, const char* value
#if INI_HANDLER_LINENO
	, int lineno
#endif // INI_HANDLER_LINENO
)
{
	if (!section || !key)
	{
		return true; // just ignore and continue
	}
	LOGD("[%s] %s=%s", section, key, value);
#define INI_MATCH(s, k) (strcmp(section, (s)) == 0 && strcmp(key, (k)) == 0)
	if (INI_MATCH("config", "nNum1"))
	{
		LOGD("  ~~ hi ~~ detect nNum1=%d", atoi(value));
	}
	return true;
}

static int ini_reader_test()
{
	int ret = ini_reader_parse_string(test_ini_str, my_ini_reader_handler, NULL);
	if (ret)
	{
		LOGE("parse ini string occurrd error. %d", ret);
		return ret;
	}
	ASSERT(0 == ret);
	LOGI("succeed parse ini string.");
	return ret;
}

//==============================================================

static int my_ini_parser_handler(const char* section,
	const char* key, const char* value, const void* user)
{
	LOGD("[%s] %s=%s", section, key, value);
	return 0;
}

static int ini_parser_test()
{
	ini_parser_code_e err;
	char buffer[256] = { 0 };
	ini_parser_handle parser = ini_parser_parse_str(test_ini_str);
	if (!parser)
	{
		LOGE("failed parse ini string");
		return 1;
	}
	LOGD("now foreach ini. %p", parser);
	err = ini_parser_foreach(parser, my_ini_parser_handler, NULL);
	ASSERT(err == INI_PARSER_CODE_SUCCEED);
	LOGD("foreach section done. %d", err);

	err = ini_parser_has_section(parser, "config");
	LOGD("have \"config\" section: %s", err == INI_PARSER_CODE_SUCCEED ? "true" : "false");
	ASSERT(err == INI_PARSER_CODE_SUCCEED);

	err = ini_parser_has_section_key(parser, "config", "test");
	LOGD("have \"[config] test\" section_key: %s", err == INI_PARSER_CODE_SUCCEED ? "true" : "false");
	ASSERT(err == INI_PARSER_CODE_SUCCEED);

	double nNum = 0.0;
	err = ini_parser_get_double(parser, "config", "nNum3", &nNum);
	if (err == INI_PARSER_CODE_SUCCEED)
	{
		LOGI("succeed get nNum3=%.3f", nNum);
	}
	else
	{
		LOGE("failed get nNum3.  %d", err);
	}
	float num3 = 0.0f;
	err = ini_parser_get_float(parser, "config", "nNum3", &num3);
	LOGI("%d get nNum3=%.3f", err, num3);

	long long num4 = 0, num5 = 0;
	err = ini_parser_get_long_long(parser, "config", "nNum4", &num4);
	ASSERT(INI_PARSER_CODE_SUCCEED == err);
	LOGI("num4=%lld", num4);
	err = ini_parser_get_long_long(parser, "config", "nNum5", &num5);
	ASSERT(INI_PARSER_CODE_SUCCEED == err);
	LOGI("num5=%lld", num5);
	ASSERT(num4 == num5);

	buffer[0] = 'a';
	buffer[1] = '\0';
	err = ini_parser_get_string(parser, "config", "test", buffer, sizeof(buffer));
	LOGI("%d get test=%s", err, buffer);
	ASSERT(buffer[0] == '\0');// because \"test\" have empty value.

	bool bool_result = true;
	err = ini_parser_get_bool(parser, "config2", "auto_start", &bool_result);
	LOGI("%d get auto_start=%d", err, bool_result);
	ASSERT(bool_result == false);

	err = ini_parser_get_bool(parser, "config2", "enable_state", &bool_result);
	LOGI("%d get enable_state=%d", err, bool_result);
	ASSERT(bool_result == true);

	err = ini_parser_get_bool(parser, "config2", "number_bool_state", &bool_result);
	LOGI("%d get number_bool_state=%d", err, bool_result);
	ASSERT(bool_result == false);

	err = ini_parser_get_string(parser, "config3", "path", buffer, sizeof(buffer));
	if (err == INI_PARSER_CODE_SUCCEED)
	{
		LOGI("succeed get path=%s", buffer);
	}
	else
	{
		LOGE("failed(%d) get path", err);
	}

	size_t buffer_mem_size = sizeof(buffer);
	if (INI_PARSER_CODE_SUCCEED == ini_parser_dump_to_mem(parser, buffer, &buffer_mem_size))
	{
		LOGD("dump ini(%zu): \n%s", buffer_mem_size, buffer);
	}
	else
	{
		LOGE("failed of dump ini to mem");
	}

	err = ini_parser_put_string(parser, "config", "new_key", "new_value");
	LOGD("%s on put new config", err == INI_PARSER_CODE_SUCCEED ? "succeed" : "failed");
	err = ini_parser_delete_by_section_key(parser, "config", "test");
	LOGD("%s on delete [config] test", err == INI_PARSER_CODE_SUCCEED ? "succeed" : "failed");
	err = ini_parser_delete_section(parser, "config4");
	LOGD("%s on delete [config4]", err == INI_PARSER_CODE_SUCCEED ? "succeed" : "failed");

	//dump string should free after use.
	char* ini_dump = ini_parser_dump(parser);
	ASSERT(ini_dump);
	if (ini_dump)
	{
		LOGI("succeed dump ini:\n%s", ini_dump);
		free(ini_dump);
	}
	err = ini_parser_save(parser, "d:/test.ini");
	ASSERT(err == INI_PARSER_CODE_SUCCEED);

	ini_parser_destroy(&parser);
	ASSERT(NULL == parser);
	return 0;
}

int ini_test()
{
	int ret = 0;
	LOGD("  --> now run ini_reader_test");
	ret = ini_reader_test();
	LOGD("  <-- ini_reader_test result: %d", ret);
	if (ret)
	{
		return ret;
	}
	LOGD(LOG_STAR_LINE "%d", ret);

	LOGD("  --> now run ini_parser_test");
	ret = ini_parser_test();
	LOGD("  <-- ini_parser_test result: %d", ret);
	return ret;
}
