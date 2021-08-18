#include "time/time_util.h"
#include "mem/strings.h"
#include <stdio.h>
#include "common_macro.h"

#define USE_TIME_CAHE   (1)

#define TIME_STAMP_FORMAT ("%m-%d %H:%M:%S")
#define TIME_STAMP_FORMAT_FOR_FILE_NAME ("%m-%d %H_%M_%S")

#if(defined(USE_TIME_CAHE) && USE_TIME_CAHE)
#define GET_TIME_FORMAT_TYPE(fmt) ((fmt[8] == ':') ? 0 : 1) 
//static char const* g_time_formats[] = { TIME_STAMP_FORMAT, TIME_STAMP_FORMAT_FOR_FILE_NAME };
typedef enum 
{
	TIME_FMT_WRITE_CACHE_STATUS_IDLE = 0,
	TIME_FMT_WRITE_CACHE_STATUS_WRITING 
}time_fmt_write_cache_status;

typedef struct
{
	long    tv_sec;
	int     timezone_hour;
	char    format_cache[TIME_STR_SIZE];
	volatile time_fmt_write_cache_status write_status;
}time_cache_t;

static time_cache_t g_time_caches[2] = { 0 };
#endif // USE_TIME_CAHE

#ifdef _WIN32

// gettimeofday taken from https://doxygen.postgresql.org/gettimeofday_8c_source.html

/* FILETIME of Jan 1 1970 00:00:00, the PostgreSQL epoch */
static const unsigned __int64 epoch = 116444736000000000UL;

/*
 * FILETIME represents the number of 100-nanosecond intervals since
 * January 1, 1601 (UTC).
 */
#define FILETIME_UNITS_PER_SEC  10000000L
#define FILETIME_UNITS_PER_USEC 10

 /*
  * Both GetSystemTimeAsFileTime and GetSystemTimePreciseAsFileTime share a
  * signature, so we can just store a pointer to whichever we find. This
  * is the pointer's type.
  */
typedef VOID(WINAPI* LcuGetSystemTimeFn) (LPFILETIME);

/* One-time initializer function, must match that signature. */
static void WINAPI init_gettimeofday(LPFILETIME lpSystemTimeAsFileTime);

/* Storage for the function we pick at runtime */
static LcuGetSystemTimeFn lcu_get_system_time = &init_gettimeofday;

/*
 * One time initializer.  Determine whether GetSystemTimePreciseAsFileTime
 * is available and if so, plan to use it; if not, fall back to
 * GetSystemTimeAsFileTime.
 */
static void WINAPI init_gettimeofday(LPFILETIME lpSystemTimeAsFileTime)
{
	/*
	 * Because it's guaranteed that kernel32.dll will be linked into our
	 * address space already, we don't need to LoadLibrary it and worry about
	 * closing it afterwards, so we're not using Pg's dlopen/dlsym() wrapper.
	 *
	 * We'll just look up the address of GetSystemTimePreciseAsFileTime if
	 * present.
	 *
	 * While we could look up the Windows version and skip this on Windows
	 * versions below Windows 8 / Windows Server 2012 there isn't much point,
	 * and determining the windows version is its self somewhat Windows
	 * version and development SDK specific...
	 */
	HMODULE kernel32 = GetModuleHandle(TEXT("kernel32.dll"));
	if (kernel32 == NULL || (lcu_get_system_time = (LcuGetSystemTimeFn)GetProcAddress(kernel32,
		"GetSystemTimePreciseAsFileTime")) == NULL)
	{
		/*
		 * The expected error from GetLastError() is ERROR_PROC_NOT_FOUND, if
		 * the function isn't present. No other error should occur.
		 *
		 * We can't report an error here because this might be running in
		 * frontend code; and even if we're in the backend, it's too early to
		 * elog(...) if we get some unexpected error.  Also, it's not a
		 * serious problem, so just silently fall back to
		 * GetSystemTimeAsFileTime irrespective of why the failure occurred.
		 */
		lcu_get_system_time = &GetSystemTimeAsFileTime;
	}

	(*lcu_get_system_time) (lpSystemTimeAsFileTime);
}

/*
 * timezone information is stored outside the kernel so tzp isn't used anymore.
 *
 * Note: this function is not for Win32 high precision timing purposes. See
 * elapsed_time().
 */
int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
	UNUSED(tzp);
	FILETIME    file_time = {0};
	ULARGE_INTEGER ularge = {0};

	(*lcu_get_system_time) (&file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	tp->tv_sec = (long)((ularge.QuadPart - epoch) / FILETIME_UNITS_PER_SEC);
	tp->tv_usec = (long)(((ularge.QuadPart - epoch) % FILETIME_UNITS_PER_SEC)
		/ FILETIME_UNITS_PER_USEC);

	return 0;
}

#endif // _WIN32

int time_util_zone_offset_seconds_to_utc()
{
	time_t rawtime = time(NULL);
	struct tm buf;

#if defined(WIN32)
	gmtime_s(&buf, &rawtime);
#else
	gmtime_r(&rawtime, &buf);
#endif
	// Request that mktime() looksup dst in timezone database
	buf.tm_isdst = -1;
	time_t gmt = mktime(&buf);

	return (int)difftime(rawtime, gmt);//seconds
}

/**
 * use fast_second2date instead of localtime_r, because locatime_r have performance issue on multi-thread.
 * taken from https://www.cnblogs.com/westfly/p/5139645.html
 */
int time_util_fast_second2date(const time_t* p_unix_sec, struct tm* lt, int timezone_hour)
{
	static const int kHoursInDay = 24;
	static const int kMinutesInHour = 60;
	static const int kDaysFromUnixTime = 2472632;
	static const int kDaysFromYear = 153;
	static const int kMagicUnkonwnFirst = 146097;
	static const int kMagicUnkonwnSec = 1461;
	lt->tm_sec = (*p_unix_sec) % kMinutesInHour;
	int i = ((int)(*p_unix_sec) / kMinutesInHour);
	lt->tm_min = i % kMinutesInHour; //nn
	i /= kMinutesInHour;
	lt->tm_hour = (i + timezone_hour) % kHoursInDay; // hh
	lt->tm_mday = (i + timezone_hour) / kHoursInDay;
	int a = lt->tm_mday + kDaysFromUnixTime;
	int b = (a * 4 + 3) / kMagicUnkonwnFirst;
	int c = (-b * kMagicUnkonwnFirst) / 4 + a;
	int d = ((c * 4 + 3) / kMagicUnkonwnSec);
	int e = -d * kMagicUnkonwnSec;
	e = e / 4 + c;
	int m = (5 * e + 2) / kDaysFromYear;
	lt->tm_mday = -(kDaysFromYear * m + 2) / 5 + e + 1;
	lt->tm_mon = (-m / 10) * 12 + m + 2;
	lt->tm_year = b * 100 + d - 6700 + (m / 10);
	return 0;
}

static inline int get_time_str(struct timeval *tval_p, char str[TIME_STR_SIZE], const char* time_format, const int timezone_hour)
{
	int ftime_len = 0;
	time_t cur_time = (time_t)(tval_p->tv_sec);

#if(defined(USE_TIME_CAHE) && USE_TIME_CAHE)
	time_cache_t *cache_p = &g_time_caches[GET_TIME_FORMAT_TYPE(time_format)];
	if (cache_p->timezone_hour == timezone_hour && cache_p->tv_sec == tval_p->tv_sec)
	{	// hit cache, just copy cached time string
		ftime_len = strlcpy(str, cache_p->format_cache, TIME_STR_SIZE);
	}
	else
	{
#endif // USE_TIME_CAHE
		struct tm lt;
		//localtime_r((const time_t*)(&cur_time), &lt);//here has performance issues on multithread.
		time_util_fast_second2date((const time_t*)(&cur_time), &lt, timezone_hour);
		ftime_len = (int)strftime(str, TIME_STR_SIZE, time_format, &lt);
#if(defined(USE_TIME_CAHE) && USE_TIME_CAHE)
		if (cache_p->timezone_hour == 0)
		{	// only cache one timezone
			cache_p->timezone_hour = timezone_hour;
		}
		if (cache_p->timezone_hour == timezone_hour && cache_p->tv_sec != tval_p->tv_sec &&
			cache_p->write_status == TIME_FMT_WRITE_CACHE_STATUS_IDLE)
		{
			cache_p->write_status = TIME_FMT_WRITE_CACHE_STATUS_WRITING;
			strlcpy(cache_p->format_cache, str, TIME_STR_SIZE);
			cache_p->tv_sec = tval_p->tv_sec;
			cache_p->write_status = TIME_FMT_WRITE_CACHE_STATUS_IDLE;
		}
	}
#endif // USE_TIME_CAHE
	ftime_len += snprintf(str + ftime_len, TIME_STR_SIZE - ftime_len, ".%03ld", tval_p->tv_usec / 1000);
	return ftime_len;
}

int time_util_get_time_str(struct timeval* tval_p, char str[TIME_STR_SIZE], int timezone_hour)
{
	return get_time_str(tval_p, str, TIME_STAMP_FORMAT, timezone_hour);
}

int time_util_get_time_str_current(char str[TIME_STR_SIZE], int timezone_hour)
{
	struct timeval tv;
	gettimeofday(&tv, NULL); // get current time
	return get_time_str(&tv, str, TIME_STAMP_FORMAT, timezone_hour);
}

int time_util_get_time_str_for_file_name(struct timeval* tval_p, char str[TIME_STR_SIZE], int timezone_hour)
{
	return get_time_str(tval_p, str, TIME_STAMP_FORMAT_FOR_FILE_NAME, timezone_hour);
}

int time_util_get_time_str_for_file_name_current(char str[TIME_STR_SIZE], int timezone_hour)
{
	struct timeval tv;
	gettimeofday(&tv, NULL); // get current time
	return get_time_str(&tv, str, TIME_STAMP_FORMAT_FOR_FILE_NAME, timezone_hour);
}

void time_util_current_ms(uint64_t* p_cur_ms)
{
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	*p_cur_ms = (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000);
}

void time_util_query_performance_ms(uint64_t* p_cur_ms)
{
#ifdef _WIN32
	//although here we have thread safe issue, because we are not lock this static variable,
	//but i think it is acceptable, it is ok to set frequency value twice.
	static uint64_t mill_frequency = 0;
	if (mill_frequency == 0)
	{
		LARGE_INTEGER frequency = { 0 }; // how many clock period on one seconds.
		QueryPerformanceFrequency(&frequency);
		mill_frequency = frequency.QuadPart / 1000;
		ASSERT(mill_frequency > 0);
	}
	STATIC_ASSERT(sizeof(LARGE_INTEGER) == sizeof(uint64_t));
	QueryPerformanceCounter((LARGE_INTEGER*)p_cur_ms);
	*p_cur_ms = (*p_cur_ms) / mill_frequency;
#else
	time_util_current_ms(p_cur_ms);
#endif // _WIN32
}
