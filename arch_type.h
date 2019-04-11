
#ifdef __amd64
typedef unsigned long long opt_type;
#define PRINT_64
#else
typedef unsigned int opt_type;
#endif

#ifdef	PRINT_64
#define OPT_FMT "%016llX"
#else
#define OPT_FMT "%08X"
#endif

