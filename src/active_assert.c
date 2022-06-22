#include <active.h>

typedef void (*activeAssertHandler)(Active_AssertInfo *);

/* Weak assert handler. Can be replaced during linking by application.
Active_AssertInfo* is a pointer to an auto variable; the aplication needs to do a copy of any members of interest */
void ACTIVE_WEAK Active_assertHandler(Active_AssertInfo *assertinfo)
{
  while (1)
    ;
}

void Active_assertHandler2(Active_AssertInfo *assertinfo)
{
  while (1)
    ;
}

static const activeAssertHandler handler = ACT_ASSERT_FN;

void Active_assert(void *pc, void *lr, char *test, char *file, uint32_t line)
{
  Active_AssertInfo a = {
      .pc = pc,
      .lr = lr,
      .test = test,
      .file = file,
      .line = line};

  handler(&a);
}