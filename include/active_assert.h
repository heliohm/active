#ifndef _ACTIVE_ASSERT_H_
#define _ACTIVE_ASSERT_H

#include <stdint.h>
#include <stddef.h>

#include <active_config_loader.h>
#include <active_port.h>

/**
 * @brief @internal - Used by Active assert module to call application assert handler for Active
 *
 */
void Active_assert(void *pc, void *lr, char *test, char *file, uint32_t line);

typedef struct active_assertinfo
{
  void *pc;      /* CPU program counter */
  void *lr;      /* CPU return address */
  char *test;    /* Test that failed assert */
  char *file;    /* Name of file */
  uint32_t line; /* Line of file */
} Active_AssertInfo;

#if ACT_ASSERT_ENABLE == 1 && defined(ACT_ASSERT_LEVEL)

#if ACT_ASSERT_LEVEL == ACT_ASSERT_LEVEL_MIN
#define ACT_ASSERT_IMPL(test, errMsg, file, line, ...) \
  do                                                   \
  {                                                    \
    if (!(test))                                       \
    {                                                  \
      Active_assert(NULL, NULL, NULL, NULL, 0);        \
    }                                                  \
  } while (0)
#elif ACT_ASSERT_LEVEL == ACT_ASSERT_LEVEL_STD
#define ACT_ASSERT_IMPL(test, errMsg, file, line, ...) \
  do                                                   \
  {                                                    \
    if (!(test))                                       \
    {                                                  \
      void *lr = ACT_GET_RETURN_ADDRESS();             \
      void *pc = ACT_GET_PROGRAM_COUNTER();            \
      Active_assert(pc, lr, NULL, NULL, line);         \
    }                                                  \
  } while (0)
#elif ACT_ASSERT_LEVEL == ACT_ASSERT_LEVEL_FULL
#define ACT_ASSERT_IMPL(test, errMsg, file, line, ...) \
  do                                                   \
  {                                                    \
    if (!(test))                                       \
    {                                                  \
      void *lr = ACT_GET_RETURN_ADDRESS();             \
      void *pc = ACT_GET_PROGRAM_COUNTER();            \
      Active_assert(pc, lr, #errMsg, file, line);      \
    }                                                  \
  } while (0)
#else
#error "Invalid assert level"
#endif /* ACT_ASSERT_LEVEL == ACT_ASSERT_LEVEL_MIN */
#else

#define ACT_ASSERT_IMPL(test, errMsg, file, line, ...)

#endif /* defined(ACT_ASSERT_ENABLE) && defined(ACT_ASSERT_LEVEL) */

#define ACT_ASSERT(test, errMsg, ...) ACT_ASSERT_IMPL(test, errMsg, ACT_GET_FILE(), ACT_GET_LINE(), ##__VA_ARGS__)

#endif /* _ACTIVE_ASSERT_H */
