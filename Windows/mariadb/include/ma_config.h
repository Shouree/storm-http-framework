
/*
 * Include file constants (processed in LibmysqlIncludeFiles.txt 1
 */
// #define HAVE_OPENSSL_APPLINK_C 1
// #define HAVE_ALLOCA_H 1
// #define HAVE_BIGENDIAN 1
// #define HAVE_SETLOCALE 1
#define HAVE_NL_LANGINFO 1
#define HAVE_DLFCN_H 1
#define HAVE_FCNTL_H 1
#define HAVE_FLOAT_H 1
#define HAVE_LIMITS_H 1
// #define HAVE_LINUX_LIMITS_H 1
// #define HAVE_PWD_H 1
#define HAVE_SELECT_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_SYS_SELECT_H 1
// #define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_STREAM_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_SYSCTL_H 1
#define HAVE_SYS_TYPES_H 1
// #define HAVE_SYS_UN_H 1
// #define HAVE_UNISTD_H 1
// #define HAVE_UCONTEXT_H 1

/*
 * function definitions - processed in LibmysqlFunctions.txt 
 */

#define HAVE_DLERROR 1
#define HAVE_DLOPEN 1
#define HAVE_GETPWUID 1
#define HAVE_MEMCPY 1
#define HAVE_POLL 1
#define HAVE_STRTOK_R 1
#define HAVE_STRTOL 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOUL 1
#define HAVE_STRTOULL 1
#define HAVE_TELL 1
#define HAVE_THR_SETCONCURRENCY 1
#define HAVE_THR_YIELD 1
#define HAVE_VASPRINTF 1
#define HAVE_VSNPRINTF 1
#define HAVE_CUSERID 1

/*
 * types and sizes
 */

#ifdef _WIN64
#define SIZEOF_CHARP 8
#else
#define SIZEOF_CHARP 4
#endif

// #define SIZEOF_CHARP sizeof(char *)
#if defined(SIZEOF_CHARP)
# define HAVE_CHARP 1
#endif


#define SIZEOF_INT 4 // True for Windows
#if defined(SIZEOF_INT)
# define HAVE_INT 1
#endif

#define SIZEOF_LONG 4 // True for Windows
#if defined(SIZEOF_LONG)
# define HAVE_LONG 1
#endif

#define SIZEOF_LONG_LONG 8 // True for Windows
#if defined(SIZEOF_LONG_LONG)
# define HAVE_LONG_LONG 1
#endif


#define SIZEOF_SIZE_T SIZEOF_CHARP // True for Windows
#if defined(SIZEOF_SIZE_T)
# define HAVE_SIZE_T 1
#endif


// #define SIZEOF_UINT sizeof(unsigned int)
#if defined(SIZEOF_UINT)
# define HAVE_UINT 1
#endif

// #define SIZEOF_USHORT sizeof(unsigned short)
#if defined(SIZEOF_USHORT)
# define HAVE_USHORT 1
#endif

// #define SIZEOF_ULONG sizeof(unsigned long)
#if defined(SIZEOF_ULONG)
# define HAVE_ULONG 1
#endif

// #define SIZEOF_INT8 @SIZEOF_INT8@
#if defined(SIZEOF_INT8)
# define HAVE_INT8 1
#endif
#define SIZEOF_UINT8 @SIZEOF_UINT8@
#if defined(SIZEOF_UINT8)
# define HAVE_UINT8 1
#endif

// #define SIZEOF_INT16 @SIZEOF_INT16@
#if defined(SIZEOF_INT16)
# define HAVE_INT16 1
#endif
// #define SIZEOF_UINT16 @SIZEOF_UINT16@
#if defined(SIZEOF_UINT16)
# define HAVE_UINT16 1
#endif

// #define SIZEOF_INT32 @SIZEOF_INT32@
#if defined(SIZEOF_INT32)
# define HAVE_INT32 1
#endif
// #define SIZEOF_UINT32 @SIZEOF_UINT32@
#if defined(SIZEOF_UINT32)
# define HAVE_UINT32 1
#endif

// #define SIZEOF_INT64 @SIZEOF_INT64@
#if defined(SIZEOF_INT64)
# define HAVE_INT64 1
#endif
// #define SIZEOF_UINT64 @SIZEOF_UINT64@
#if defined(SIZEOF_UINT64)
# define HAVE_UINT64 1
#endif

#define SIZEOF_SOCKLEN_T @SIZEOF_SOCKLEN_T@
#if defined(SIZEOF_SOCKLEN_T)
# define HAVE_SOCKLEN_T 1
#endif

#define SOCKET_SIZE_TYPE int

#define LOCAL_INFILE_MODE_OFF  0
#define LOCAL_INFILE_MODE_ON   1
#define LOCAL_INFILE_MODE_AUTO 2
#define ENABLED_LOCAL_INFILE LOCAL_INFILE_MODE_AUTO

#define MARIADB_DEFAULT_CHARSET "utf8mb4"

