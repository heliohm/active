#ifndef _ACTIVE_CONFIG_LOADER_H_
#define _ACTIVE_CONFIG_LOADER_H_

#if defined __has_include
#if __has_include(<active_config.h>)
#include <active_config.h>
#else
#warning "active_config.h file not found during compilation, using minimal defaults for tests. Check include directories."
#endif
#else
#warning __has_include macro note supported in compiler. Edit this file to configure library.
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

/**
 * @brief Active debug printing with ACT_DBGPRINT() - set to 0 to disable
 *
 */

#ifndef ACT_CFG_DEBUG_PRINT
#define ACT_CFG_DEBUG_PRINT 1
#endif

#endif /* _ACTIVE_CONFIG_LOADER_H */