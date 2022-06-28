#include <active.h>

typedef void (*activeAssertHandler)(Active_AssertInfo *);

/* Default assert handler. Can be replaced in application active_config.h
Active_AssertInfo* is a pointer to an auto variable; the aplication needs to do a copy of any members of interest */
void Active_assertHandler(Active_AssertInfo *assertinfo)
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