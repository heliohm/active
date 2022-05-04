#include <active.h>
#include <unity.h>

static void ao_emptyDispatch(Active *me, Event const *const e)
{
}

static ACTIVE_QBUF(testQBuf, 1);
static ACTIVE_Q(testQ);
static ACTIVE_THREAD(testT);
static ACTIVE_THREAD_STACK(testTStack, 512);
static ACTIVE_THREAD_STACK_SIZE(testTStackSz, testTStack);

const static queueData qdtest = {.maxMsg = 10,
                                 .queBuf = testQBuf,
                                 .queue = &testQ};

const static threadData tdtest = {.thread = &testT,
                                  .pri = 1,
                                  .stack = testTStack,
                                  .stack_size = testTStackSz};

static bool wasInitSigReceived = false;

static void ao_initDispatch(Active *me, Event const *const e)
{
  if (e->type == SIGNAL && EVT_CAST(e, Signal)->sig == START_SIG)
  {
    wasInitSigReceived = true;
  }
}

static void test_function_active_init()
{
  Active ao;
  Active_init(&ao, ao_emptyDispatch);

  TEST_ASSERT_EQUAL(ao_emptyDispatch, ao.dispatch);
}

static void test_function_active_start()
{
  Active ao;
  Active_init(&ao, ao_initDispatch);
  Active_start(&ao, &qdtest, &tdtest);
  k_msleep(50);
  TEST_ASSERT_TRUE(wasInitSigReceived);
}

void main()
{
  k_msleep(2000);

  UNITY_BEGIN();

  RUN_TEST(test_function_active_init);
  RUN_TEST(test_function_active_start);

  UNITY_END();
}