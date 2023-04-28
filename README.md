# Brief
  What's this: This is a common c utils library (works for unix(android)/windows).
  > note: api may change frequently at this stage. see commits log for more details

----
## Components
* **log**
  > print log to file/console/logcat(if on Android) at the same time, of course you can config it.
  
  *  **logger** : a logger facade. xlog and slog implement it
  *  **xlog** : support print level/tag/tid/time with log message
  *  **slog** : simple log, only print level/tag with log message

* **thread**
  > posix style thread/semaphore api on windows(thanks for [pthread-win32](https://sourceforge.net/projects/pthreads4w/)). 
  also works on unix(linux/android).

* **data**
  > data structure : include ***array***, ***hashmap***, ***list***, ***base64*** ...

* **file**
  > file util: include **file_util_read/write**, **ini_reader/parser** ...

* **memory**
   *  **string** : include ***asprintf***, ***stringbuilder***, ***str_params***, ***strlcpy***, ***strlcat***, ***strreplace***, ***strsplit***, ***strtrim***, ***strutf8len*** ...
   *  **allocator** : can trace heap memory, help you find memory leak or memory corruption.
   *  **mplite** : a zero-malloc memory pool based on [SQLite's memsys5 memory subsystem](https://github.com/hannes/sqlite-simplified/blob/master/mem5.c)

* **ring**
  > ringbuffer: include ***ringbuffer***, ***ring_msg_queue***, ***msg_queue_handler***, ***autocover_buffer*** ...

* **time**
  > time_util: include ***current_milliseconds***, ***fast_second2date*** ...

----
## How to build
  
  * ### common platforms
    > go to ***build***  folder, and edit build script first(**to make sure cmake/ninja/NDK(for Android) location**), then execute script to build it, it will copy the compiled result to the specified location
    
    for example:
    
    |platform     | onekey deploy build                      | manual build                                     |
    | --------    | :-----                                   | :----                                            |
    | **windows** | `deploy_for_windows.bat`                 | ` make_windows.bat Win32 Release 0 `             | 
    | **linux**   | `chmod +x *.sh && ./deploy_for_linux.sh` | ` chmod +x *.sh && ./make_linux.sh m64 Release ` |
    | **android** | `deploy_for_android.bat`                 | ` make_android.bat armeabi-v7a Release `         |
  
  * ### other platforms
    1. #### first, write cmake cross toolchain file on **build/cmake/** folder:
      > for define c/cxx compiler location and flags.
      
      example: create *hisi.toolchain.cmake* file on *build/cmake/* folder:
      ```
      SET(UNIX TRUE CACHE BOOL "")
      SET(CMAKE_SYSTEM_NAME Linux) # this one is important
      SET(CMAKE_SYSTEM_VERSION 1)  # this one not so much
      
      set(CROSS_TOOLCHAIN_PATH_PREFIX "/mnt/toolchains/hisi-linux/x86-arm/arm-himix100-linux/bin/arm-himix100-linux-")
      message(STATUS "Current CROSS_TOOLCHAIN_PATH_PREFIX is => ${CROSS_TOOLCHAIN_PATH_PREFIX}")
      if(("${CROSS_TOOLCHAIN_PATH_PREFIX}" STREQUAL  ""))
        message(FATAL_ERROR  "you should set CROSS_TOOLCHAIN_PATH_PREFIX first")
      endif()
      #set compiler location
      SET(CMAKE_C_COMPILER "${CROSS_TOOLCHAIN_PATH_PREFIX}gcc")
      SET(CMAKE_CXX_COMPILER "${CROSS_TOOLCHAIN_PATH_PREFIX}g++")
      SET(CMAKE_AR "${CROSS_TOOLCHAIN_PATH_PREFIX}ar")
      SET(CMAKE_LINKER "${CROSS_TOOLCHAIN_PATH_PREFIX}ld")
      SET(CMAKE_RANLIB "${CROSS_TOOLCHAIN_PATH_PREFIX}ranlib")
      SET(CMAKE_STRIP "${CROSS_TOOLCHAIN_PATH_PREFIX}strip")
      
      string(APPEND CMAKE_C_FLAGS          " -fPIC")
      string(APPEND CMAKE_CXX_FLAGS        " -fPIC")
      string(APPEND CMAKE_EXE_LINKER_FLAGS " -fPIC -fPIE")
      add_definitions(-D_LCU_NOT_SUPPORT_PTHREAD_SETNAME) # custom compile definitions
      ```
  
    2. #### then execute cmake build script:
      > for generate makefile and compile it
      
      example:
      ```shell
      cmake -H. -B./build_hisi -DCMAKE_TOOLCHAIN_FILE=./cmake/hisi.toolchain.cmake 
      cmake --build ./build_hisi --config Release
      ```

## How to use
  >  copy header and lib(static or shared) to your project, and link it,
  or just copy source and header file to your project.

## Want demo
  > see test folder, it demonstrate to you simple use case.

----
## Release Log

* **1.5.3**
  > 1. add logger_facade, and xlog/slog implement it
  > 2. improve xlog performance: use our method to print function name and line number
  > 3. windows dll truly export all symbols( this feature supported by cmake )

* **1.5.2**
  > 1. improve time_util_get_current_time_str performance
  > 2. improve xlog performance
  > 3. add flush mode on xlog

* **1.4.1**
  > 1. add msg_queue/msg_queue_handler: support various msg
  > 2. rename old msg_queue/msg_queue_handler to fixed_msg_queue/fixed_msg_queue_handler
  > 3. extract msg_queue_errno to header.

* **1.0.0 ~ 1.4.0**
  > Release log not write here, see commit log for more details. btw: suggest use latest code
  >    for example, ver 1.4.0 commit log url is https://github.com/wind0ws/libcutils/commits/1.4.0
  