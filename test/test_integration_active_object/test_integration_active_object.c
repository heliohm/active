#include <active.h>
#include <unity.h>

static void ao_emptyDispatch(Active *me, Event const *const e)
{
}

static ACTP_QBUF(testQBuf, 1);
static ACTP_Q(testQ);
static ACTP_THREAD(testT);
static ACTP_THREAD_STACK_DEFINE(testTStack, 512);
static ACTP_THREAD_STACK_SIZE(testTStackSz, testTStack);

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
  ACTP_init(&ao, ao_emptyDispatch, &qdtest, &tdtest);

  TEST_ASSERT_EQUAL(ao_emptyDispatch, ao.dispatch);
  TEST_ASSERT_EQUAL(qdtest.queue, ao.queue);
}

static void test_function_active_start()
{
  Active ao;
  // Todo: Refactor test to terminate thread after test
  ACTP_init(&ao, ao_initDispatch, &qdtest, &tdtest);
  ACTP_start(&ao);
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