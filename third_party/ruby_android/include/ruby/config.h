#ifndef INCLUDE_RUBY_CONFIG_H
#define INCLUDE_RUBY_CONFIG_H 1
/* confdefs.h */
#define CANONICALIZATION_FOR_MATHN 1
#define STDC_HEADERS 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STRINGS_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_UNISTD_H 1
#define __EXTENSIONS__ 1
#define _ALL_SOURCE 1
#define _GNU_SOURCE 1
#define _POSIX_PTHREAD_SEMANTICS 1
#define _TANDEM_SOURCE 1
#define RUBY_SYMBOL_EXPORT_BEGIN _Pragma("GCC visibility push(default)")
#define RUBY_SYMBOL_EXPORT_END _Pragma("GCC visibility pop")
#define HAVE_LIBDL 1
#define HAVE_DIRENT_H 1
#define HAVE__BOOL 1
#define HAVE_STDBOOL_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_LIMITS_H 1
#define HAVE_SYS_FILE_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_SYS_SYSCALL_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TIMES_H 1
#define HAVE_GRP_H 1
#define HAVE_UTIME_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_UTIME_H 1
#define HAVE_FLOAT_H 1
#define HAVE_UCONTEXT_H 1
#define HAVE_LOCALE_H 1
#define HAVE_SYS_SENDFILE_H 1
#define HAVE_TIME_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_PRCTL_H 1
#define HAVE_SYS_UIO_H 1
#define HAVE_MALLOC_H 1
#define HAVE_TYPEOF 1
#define typeof __typeof__
#define HAVE_LONG_LONG 1
#define HAVE_OFF_T 1
// Please just use stdint..
// I don't understand why all these projects are
// obsessed with their custom data types
#ifdef ARCH_32BIT
#define SIZEOF_INT 4
#define SIZEOF_SHORT 2
#define SIZEOF_LONG 4
#define SIZEOF_LONG_LONG 8
#define SIZEOF___INT64 0
#define SIZEOF___INT128 0
#define SIZEOF_OFF_T 4
#define SIZEOF_VOIDP 4
#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8
#define SIZEOF_TIME_T 4
#define SIZEOF_CLOCK_T 4
#else
#define SIZEOF_INT 4
#define SIZEOF_SHORT 2
#define SIZEOF_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF___INT64 0
#define SIZEOF___INT128 0
#define SIZEOF_OFF_T 8
#define SIZEOF_VOIDP 8
#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8
#define SIZEOF_TIME_T 8
#define SIZEOF_CLOCK_T 8
#endif //!ARCH_32BIT
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define PACKED_STRUCT(x) x __attribute__((packed))
#define PACKED_STRUCT_UNALIGNED(x) x
#define PRI_LL_PREFIX "ll"
#define rb_pid_t pid_t
#define SIGNEDNESS_OF_PID_T -1
#define PIDT2NUM(v) INT2NUM(v)
#define NUM2PIDT(v) NUM2INT(v)
#define PRI_PIDT_PREFIX PRI_INT_PREFIX
#define rb_uid_t uid_t
#define SIGNEDNESS_OF_UID_T +1
#define UIDT2NUM(v) UINT2NUM(v)
#define NUM2UIDT(v) NUM2UINT(v)
#define PRI_UIDT_PREFIX PRI_INT_PREFIX
#define rb_gid_t gid_t
#define SIGNEDNESS_OF_GID_T +1
#define GIDT2NUM(v) UINT2NUM(v)
#define NUM2GIDT(v) NUM2UINT(v)
#define PRI_GIDT_PREFIX PRI_INT_PREFIX
#define rb_time_t time_t
#define SIGNEDNESS_OF_TIME_T -1
#define TIMET2NUM(v) LONG2NUM(v)
#define NUM2TIMET(v) NUM2LONG(v)
#define PRI_TIMET_PREFIX PRI_LONG_PREFIX
#define rb_dev_t dev_t
#define SIGNEDNESS_OF_DEV_T +1
#define DEVT2NUM(v) UINT2NUM(v)
#define NUM2DEVT(v) NUM2UINT(v)
#define PRI_DEVT_PREFIX PRI_INT_PREFIX
#define rb_mode_t mode_t
#define SIGNEDNESS_OF_MODE_T +1
#define MODET2NUM(v) UINT2NUM(v)
#define NUM2MODET(v) NUM2UINT(v)
#define PRI_MODET_PREFIX PRI_INT_PREFIX
#define rb_rlim_t rlim_t
#define SIGNEDNESS_OF_RLIM_T +1
#define RLIM2NUM(v) ULONG2NUM(v)
#define NUM2RLIM(v) NUM2ULONG(v)
#define PRI_RLIM_PREFIX PRI_LONG_PREFIX
#define rb_off_t off_t
#define SIGNEDNESS_OF_OFF_T -1
#define OFFT2NUM(v) LONG2NUM(v)
#define NUM2OFFT(v) NUM2LONG(v)
#define PRI_OFFT_PREFIX PRI_LONG_PREFIX
#define rb_clockid_t clockid_t
#define SIGNEDNESS_OF_CLOCKID_T -1
#define CLOCKID2NUM(v) INT2NUM(v)
#define NUM2CLOCKID(v) NUM2INT(v)
#define PRI_CLOCKID_PREFIX PRI_INT_PREFIX
#define HAVE_PROTOTYPES 1
#define TOKEN_PASTE(x,y) x##y
#define STRINGIZE(expr) STRINGIZE0(expr)
#define HAVE_STDARG_PROTOTYPES 1
#define HAVE_VA_ARGS_MACRO 1
#define NORETURN(x) __attribute__ ((noreturn)) x
#define DEPRECATED(x) __attribute__ ((deprecated)) x
#define NOINLINE(x) __attribute__ ((noinline)) x
#define FUNC_MINIMIZED(x) __attribute__ ((optimize("-Os","-fomit-frame-pointer"))) x
#define HAVE_ATTRIBUTE_FUNCTION_ALIAS 1
#define RUBY_ALIAS_FUNCTION_TYPE(type, prot, name, args) type prot __attribute__((alias(#name)));
#define RUBY_ALIAS_FUNCTION_VOID(prot, name, args) RUBY_ALIAS_FUNCTION_TYPE(void, prot, name, args)
#define HAVE_GCC_ATOMIC_BUILTINS 1
#define HAVE_GCC_SYNC_BUILTINS 1
#define UNREACHABLE __builtin_unreachable()
#define RUBY_FUNC_EXPORTED __attribute__ ((visibility("default"))) extern
#define RUBY_FUNCTION_NAME_STRING __func__
#define HAVE_DECL_SYS_NERR 0
#define HAVE_DECL_GETENV 1
#define SIZEOF_SIZE_T 4
#define SIZEOF_PTRDIFF_T 4
#define PRI_SIZE_PREFIX "z"
#define PRI_PTRDIFF_PREFIX "t"
#define HAVE_STRUCT_STAT_ST_BLKSIZE 1
#define HAVE_ST_BLKSIZE 1
#define HAVE_STRUCT_STAT_ST_BLOCKS 1
#define HAVE_ST_BLOCKS 1
#define HAVE_STRUCT_STAT_ST_RDEV 1
#define HAVE_ST_RDEV 1
#define SIZEOF_STRUCT_STAT_ST_SIZE SIZEOF_LONG_LONG
#define SIZEOF_STRUCT_STAT_ST_BLOCKS SIZEOF_LONG_LONG
#define SIZEOF_STRUCT_STAT_ST_INO SIZEOF_LONG_LONG
#define HAVE_STRUCT_STAT_ST_ATIMENSEC 1
#define HAVE_STRUCT_STAT_ST_MTIMENSEC 1
#define HAVE_STRUCT_STAT_ST_CTIMENSEC 1
#define HAVE_STRUCT_TIMEVAL 1
#define SIZEOF_STRUCT_TIMEVAL_TV_SEC SIZEOF_TIME_T
#define HAVE_STRUCT_TIMESPEC 1
#define HAVE_STRUCT_TIMEZONE 1
#define HAVE_CLOCKID_T 1
#define HAVE_INT8_T 1
#define SIZEOF_INT8_T 1
#define HAVE_UINT8_T 1
#define SIZEOF_UINT8_T 1
#define HAVE_INT16_T 1
#define SIZEOF_INT16_T 2
#define HAVE_UINT16_T 1
#define SIZEOF_UINT16_T 2
#define HAVE_INT32_T 1
#define SIZEOF_INT32_T 4
#define HAVE_UINT32_T 1
#define SIZEOF_UINT32_T 4
#define HAVE_INT64_T 1
#define SIZEOF_INT64_T 8
#define HAVE_UINT64_T 1
#define SIZEOF_UINT64_T 8
#define HAVE_INTPTR_T 1
#define SIZEOF_INTPTR_T 4
#define HAVE_UINTPTR_T 1
#define SIZEOF_UINTPTR_T 4
#define HAVE_SSIZE_T 1
#define SIZEOF_SSIZE_T 4
#define GETGROUPS_T gid_t
#define RETSIGTYPE void
#define HAVE_ALLOCA_H 1
#define HAVE_ALLOCA 1
#define HAVE_ACOSH 1
#define HAVE_CBRT 1
#define HAVE_DUP2 1
#define HAVE_ERF 1
#define HAVE_FFS 1
#define HAVE_FINITE 1
#define HAVE_FLOCK 1
#define HAVE_HYPOT 1
#define HAVE_ISINF 1
#define HAVE_ISNAN 1
#define HAVE_LGAMMA_R 1
#define HAVE_MEMMOVE 1
#define HAVE_NEXTAFTER 1
#define HAVE_STRCHR 1
#define HAVE_STRERROR 1
#define HAVE_STRLCAT 1
#define HAVE_STRLCPY 1
#define HAVE_STRSTR 1
#define HAVE_TGAMMA 1
#define SPT_TYPE SPT_REUSEARGV
#define HAVE_SIGNBIT 1
#define HAVE_FORK 1
#define HAVE_VFORK 1
#define HAVE_WORKING_VFORK 1
#define HAVE_WORKING_FORK 1
#define HAVE__LONGJMP 1
#define HAVE__SETJMP 1
#define HAVE_ATAN2L 1
#define HAVE_ATAN2F 1
#define HAVE_CHROOT 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_COSH 1
#define HAVE_DIRFD 1
#define HAVE_DL_ITERATE_PHDR 1
#define HAVE_DLOPEN 1
#define HAVE_DLADDR 1
#define HAVE_DUP 1
#undef HAVE_DUP3
#define HAVE_FCHMOD 1
#define HAVE_FCHOWN 1
#define HAVE_FCNTL 1
#define HAVE_FDATASYNC 1
#define HAVE_FMOD 1
#define HAVE_FSYNC 1
#define HAVE_FTRUNCATE 1
#define HAVE_FTRUNCATE64 1
#define HAVE_GETCWD 1
#define HAVE_GETGRNAM 1
#define HAVE_GETGROUPS 1
#define HAVE_GETPGID 1
#define HAVE_GETPGRP 1
#define HAVE_GETPRIORITY 1
#undef HAVE_GETPWNAM_R
#define HAVE_GETRESGID 1
#define HAVE_GETRESUID 1
#define HAVE_GETRLIMIT 1
#undef HAVE_GETSID
#define HAVE_GETTIMEOFDAY 1
#define HAVE_GMTIME_R 1
#define HAVE_INITGROUPS 1
#define HAVE_IOCTL 1
#define HAVE_ISFINITE 1
#define HAVE_KILLPG 1
#define HAVE_LCHOWN 1
#define HAVE_LINK 1
#define HAVE_LLABS 1
#undef HAVE_LOG2
#define HAVE_LSTAT 1
#undef HAVE_MALLOC_USABLE_SIZE
#define HAVE_MEMALIGN 1
#define HAVE_WRITEV 1
#define HAVE_MEMRCHR 1
#define HAVE_MEMMEM 1
#define HAVE_MKTIME 1
#define HAVE_PIPE2 1
#define HAVE_POLL 1
#undef HAVE_POSIX_FADVISE
#undef HAVE_POSIX_MEMALIGN
#undef HAVE_PPOLL
#define HAVE_PREAD 1
#define HAVE_READLINK 1
#define HAVE_ROUND 1
#define HAVE_SCHED_GETAFFINITY 1
#define HAVE_SENDFILE 1
#define HAVE_SETEGID 1
#define HAVE_SETENV 1
#define HAVE_SETEUID 1
#define HAVE_SETGID 1
#define HAVE_SETGROUPS 1
#define HAVE_SETPGID 1
#define HAVE_SETPGRP 1
#define HAVE_SETREGID 1
#define HAVE_SETRESGID 1
#define HAVE_SETRESUID 1
#define HAVE_SETREUID 1
#define HAVE_SETRLIMIT 1
#define HAVE_SETSID 1
#define HAVE_SETUID 1
#define HAVE_SHUTDOWN 1
#define HAVE_SIGACTION 1
#define HAVE_SIGALTSTACK 1
#define HAVE_SIGPROCMASK 1
#define HAVE_SINH 1
#define HAVE_SYMLINK 1
#define HAVE_SYSCALL 1
#define HAVE_SYSCONF 1
#define HAVE_TANH 1
#define HAVE_TIMEGM 1
#define HAVE_TIMES 1
#define HAVE_TRUNCATE 1
#define HAVE_TRUNCATE64 1
#define HAVE_UNSETENV 1
#undef HAVE_UTIMENSAT
#define HAVE_UTIMES 1
#define HAVE_WAIT4 1
#define HAVE_WAITPID 1
#define HAVE_BUILTIN___BUILTIN_BSWAP16 1
#define HAVE_BUILTIN___BUILTIN_BSWAP32 1
#define HAVE_BUILTIN___BUILTIN_BSWAP64 1
#define HAVE_BUILTIN___BUILTIN_CLZ 1
#define HAVE_BUILTIN___BUILTIN_CLZL 1
#define HAVE_BUILTIN___BUILTIN_CLZLL 1
#define HAVE_BUILTIN___BUILTIN_CHOOSE_EXPR 1
#define HAVE_BUILTIN___BUILTIN_TYPES_COMPATIBLE_P 1
#define ATAN2_INF_C99 1
#define HAVE_CLOCK_GETRES 1
#define HAVE_STRUCT_TM_TM_ZONE 1
#define HAVE_TM_ZONE 1
#define HAVE_STRUCT_TM_TM_GMTOFF 1
#define HAVE_DAYLIGHT 1
#define HAVE_VAR_TIMEZONE 1
#define TYPEOF_VAR_TIMEZONE long
#define HAVE_TIMEZONE 1
#define TIMEZONE_VOID 1
#define NEGATIVE_TIME_T 1
#define LOCALTIME_OVERFLOW_PROBLEM 1
#define POSIX_SIGNAL 1
#define RSHIFT(x,y) ((x)>>(int)(y))
#define HAVE__SC_CLK_TCK 1
#define STACK_GROW_DIRECTION 0
#define _REENTRANT 1
#define _THREAD_SAFE 1
#define HAVE_LIBPTHREAD 1
#define HAVE_SCHED_YIELD 1
#define HAVE_PTHREAD_GETATTR_NP 1
#define HAVE_PTHREAD_ATTR_GETSTACK 1
#define HAVE_PTHREAD_COND_INIT 1
#undef HAVE_PTHREAD_CONDATTR_SETCLOCK
#define HAVE_PTHREAD_CONDATTR_INIT 1
#define HAVE_PTHREAD_SIGMASK 1
#define HAVE_PTHREAD_SETNAME_NP 1
#define HAVE_PTHREAD_ATTR_INIT 1
#define SET_THREAD_NAME(name) pthread_setname_np(pthread_self(), name)
#define DEFINE_MCONTEXT_PTR(mc, uc) mcontext_t *mc = &(uc)->uc_mcontext
#define USE_ELF 1
#define HAVE_ELF_H 1
#define DLEXT_MAXLEN 3
#define DLEXT ".so"
#define LIBDIR_BASENAME "lib"
#ifdef ARCH_32BIT
#define RUBY_SETJMP(env) __builtin_setjmp((env))
#define RUBY_LONGJMP(env,val) __builtin_longjmp((env),val)
#else
#define RUBY_SETJMP(env) setjmp(env)
#define RUBY_LONGJMP(env,val) longjmp((env),val)
#endif
#define RUBY_JMP_BUF jmp_buf
#define HAVE_PTHREAD_H 1
#ifdef ARCH_32BIT
#define RUBY_PLATFORM "arm-linux-androideabi"
#else
#define RUBY_PLATFORM "arm-linux-androidaarch64"
#endif // ARCH_32BIT
#endif /* INCLUDE_RUBY_CONFIG_H */
