#pragma once
#ifndef __LCU_TIME_RFC2822_H
#define __LCU_TIME_RFC2822_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define TIME_RFC2822_STR_LEN (31)

#define TIME_RFC2822_UTC_STR_LEN (29)

int time_now_rfc2822(char *out_time_str, size_t out_time_str_len);

//Format： Thu, 09 Jan 2020 06:11:13 UTC
int time_now_rfc2822_utc(char *out_time_str, size_t out_time_str_len);

#ifdef __cplusplus
};
#endif

#endif //__LCU_TIME_RFC2822_H
