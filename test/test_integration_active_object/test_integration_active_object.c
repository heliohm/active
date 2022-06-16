#include <active.h>
#include <unity.h>

static void ao_emptyDispatch(Active *me, ACT_Evt const *const e)
{
}

static ACT_QBUF(testQBuf, 1);
static ACT_Q(testQ);
static ACT_THREAD(testT);
static ACT_THREAD_STACK_DEFINE(testTStack, 512);
static ACT_THREAD_STACK_SIZE(testTStackSz, testTStack);

const static ACT_QueueData qdtest = {.maxMsg = 10,
                                     .queBuf = testQBuf,
                                     .queue = &testQ};

const static ACT_ThreadData tdtest = {.thread = &testT,
                                      .pri = 1,
                                      .stack = testTStack,
                                      .stack_size = testTStackSz};

static bool wasInitSigReceived = false;

static void ao_initDispatch(Active *me, ACT_Evt const *const e)
{
  if (e->type == ACT_SIGNAL && EVT_CAST(e, ACT_Signal)->sig == ACT_START_SIG)
  {
    wasInitSigReceived = true;
  }
}

static void test_function_active_init()
{
  Active ao;
  ACT_init(&ao, ao_emptyDispatch, &qdtest, &tdtest);

  TEST_ASSERT_EQUAL(ao_emptyDispatch, ao.dispatch);
  TEST_ASSERT_EQUAL(qdtest.queue, ao.queue);
}

static void test_function_active_start()
{
  Active ao;
  // Todo: Refactor test to terminate thread after test
  ACT_init(&ao, ao_initDispatch, &qdtest, &tdtest);
  ACT_start(&ao);
  ACT_SLEEPMS(50);
  TEST_ASSERT_TRUE(wasInitSigReceived);
}

void main()
{
  ACT_SLEEPMS(2000);

  UNITY_BEGIN();

  RUN_TEST(test_function_active_init);
  RUN_TEST(test_function_active_start);

  UNITY_END();
}