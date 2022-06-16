#include <active.h>
#include <unity.h>

static ACT_QBUF(testQBuf, 1);
static ACT_Q(testQ);
static ACT_THREAD(testT);
static ACT_THREAD_STACK_DEFINE(testTStack, 512);
static ACT_THREAD_STACK_SIZE(testTStackSz, testTStack);

const static ACT_QueueData qdtest = {.maxMsg = 1,
                                     .queBuf = testQBuf,
                                     .queue = &testQ};

const static ACT_ThreadData tdtest = {.thread = &testT,
                                      .pri = 1,
                                      .stack = testTStack,
                                      .stack_size = testTStackSz};

enum TestUserSignal
{
  TEST_SIG = ACT_USER_SIG,
  TIME_SIG
};

Active ao;
ACT_Signal testSig, timeSig;
ACT_TimEvt timeEvt;

static bool wasTestSigReceived = false, wasTimeSigReceived = false;

static void ao_dispatch(Active *me, ACT_Evt const *const e)
{
  if ((e->type == ACT_SIGNAL) && (EVT_CAST(e, ACT_Signal)->sig == TEST_SIG) && (e == EVT_UPCAST(&testSig)))
  {
    wasTestSigReceived = true;
  }

  if ((e->type == ACT_SIGNAL) && (EVT_CAST(e, ACT_Signal)->sig == TIME_SIG) && (e == EVT_UPCAST(&timeSig)))
  {
    wasTimeSigReceived = true;
  }
}

static void test_function_active_post()
{

  ACT_postEvt(&ao, EVT_UPCAST(&testSig));
  ACT_SLEEPMS(50);

  TEST_ASSERT_TRUE(wasTestSigReceived);
}

static void test_function_active_post_timeevt()
{
  ACT_TimeEvt_start(&timeEvt, 50, 0);
  ACT_SLEEPMS(100);

  TEST_ASSERT_TRUE(wasTimeSigReceived);
}

void main()
{
  ACT_SLEEPMS(2000);

  UNITY_BEGIN();

  ACT_init(&ao, ao_dispatch, &qdtest, &tdtest);
  ACT_start(&ao);

  ACT_Signal_init(&testSig, &ao, TEST_SIG);
  ACT_Signal_init(&timeSig, &ao, TIME_SIG);
  ACT_TimEvt_init(&timeEvt, &ao, EVT_UPCAST(&timeSig), &ao, NULL);

  RUN_TEST(test_function_active_post);
  RUN_TEST(test_function_active_post_timeevt);

  UNITY_END();
}