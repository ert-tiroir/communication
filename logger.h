
#define DEBUG_LEVEL 5

void start_logger (const char* type);
void end_logger ();
void slogger (const char* message);
void ilogger (int message);
void llogger (long long message);

#if DEBUG_LEVEL >= 1
#define __log_error(params) { \
    start_logger("ERR"); \
    params; \
    end_logger(); \
}
#define log_error(message) __log_error( slogger(message) );
#else
#define __log_error(params)
#define log_error(message)
#endif
#if DEBUG_LEVEL >= 2
#define __log_danger(params) { \
    start_logger("DGR"); \
    params; \
    end_logger(); \
}
#define log_danger(message) __log_danger( slogger(message) );
#else
#define __log_danger(params)
#define log_danger(message)
#endif
#if DEBUG_LEVEL >= 3
#define __log_warn(params) { \
    start_logger("WRN"); \
    params; \
    end_logger(); \
}
#define log_warn(message) __log_warn( slogger(message) );
#else
#define __log_warn(params)
#define log_warn(message)
#endif
#if DEBUG_LEVEL >= 4
#define __log_info(params) { \
    start_logger("INF"); \
    params; \
    end_logger(); \
}
#define log_info(message) __log_info( slogger(message) );
#else
#define __log_info(params)
#define log_info(message)
#endif
#if DEBUG_LEVEL >= 5
#define __log_debug(params) { \
    start_logger("DBG"); \
    params; \
    end_logger(); \
}
#define log_debug(message) __log_debug( slogger(message) );
#else
#define __log_debug(params)
#define log_debug(message)
#endif
