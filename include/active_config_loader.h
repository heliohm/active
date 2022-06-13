#ifndef _ACTIVE_CONFIG_LOADER_H_
#define _ACTIVE_CONFIG_LOADER_H_

#if defined __has_include
#if __has_include(<active_config.h>)
#include <active_config.h>
#endif
#else
#warning "active_config.h file not found during compilation, using defaults. Check __has_include macro and include directories for active_config.h"
#endif

/**
 * @brief Defines for Active memory pool sizes
 *
 */
#ifndef ACT_MEM_NUM_SIGNALS
#define ACT_MEM_NUM_SIGNALS 3
#endif

#ifndef ACT_MEM_NUM_MESSAGES
#define ACT_MEM_NUM_MESSAGES 3
#endif

#ifndef ACT_MEM_NUM_TIMEEVT
#define ACT_MEM_NUM_TIMEEVT 3
#endif
/*
#ifndef ACT_MEM_NUM_OBJPOOLS
#define ACT_MEM_NUM_OBJPOOLS 1
#endif
*/
#endif /* _ACTIVE_CONFIG_LOADER_H */