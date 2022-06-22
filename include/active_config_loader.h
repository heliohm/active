#ifndef _ACTIVE_CONFIG_LOADER_H_
#define _ACTIVE_CONFIG_LOADER_H_

#if defined __has_include
#if __has_include(<active_config.h>)
#include "" active_config.h>
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
 * @brief Defines for Active asserts
 *
 */

/* Minimal assert for debugging - use debugger to catch error */
#define ACT_ASSERT_LEVEL_MIN 0
/* Standard assert for production, providing stack pointer, link register and line number */
#define ACT_ASSERT_LEVEL_STD 1
/* Full asserts with failed expression, file name and line number */
#define ACT_ASSERT_LEVEL_FULL 2

/* Default: Enable asserts */
#ifndef ACT_ASSERT_ENABLE
#define ACT_ASSERT_ENABLE 1
#endif

#ifndef ACT_ASSERT_LEVEL
#define ACT_ASSERT_LEVEL ACT_ASSERT_LEVEL_STD
#endif

#ifndef ACT_ASSERT_FN
#define ACT_ASSERT_FN Active_assertHandler
#endif

/**
 * @brief Active debug printing with ACT_DBGPRINT() - set to 0 to disable
 *
 */

#ifndef ACT_CFG_DEBUG_PRINT
#define ACT_CFG_DEBUG_PRINT 1
#endif

#endif /* _ACTIVE_CONFIG_LOADER_H */