#include <active.h>
#include <unity.h>

static ACTP_QBUF(testQBuf, 1);
static ACTP_Q(testQ);
static ACTP_THREAD(testT);
static ACTP_THREAD_STACK_DEFINE(testTStack, 512);
static ACTP_THREAD_STACK_SIZE(testTStackSz, testTStack);

const static queueData qdtest = {.maxMsg = 1,
                                 .queBuf = testQBuf,
                                 .queue = &testQ};

const static threadData tdtest = {.thread = &testT,
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
TimeEvt timeEvt;

static bool wasTestSigReceived = false, wasTimeSigReceived = false;

static void ao_dispatch(Active *me, ACT_Evt const *const e)
{
  if ((e->type == SIGNAL) && (EVT_CAST(e, ACT_Signal)->sig == TEST_SIG) && (e == EVT_UPCAST(&testSig)))
  {
    wasTestSigReceived = true;
  }

  if ((e->type == SIGNAL) && (EVT_CAST(e, ACT_Signal)->sig == TIME_SIG) && (e == EVT_UPCAST(&timeSig)))
  {
    wasTimeSigReceived = true;
  }
}

static void test_function_active_post()
{

  ACT_post(&ao, EVT_UPCAST(&testSig));
  k_msleep(50);

  TEST_ASSERT_TRUE(wasTestSigReceived);
}

static void test_function_active_post_timeevt()
{
  ACT_TimeEvt_start(&timeEvt, 50, 0);
  k_msleep(100);

  TEST_ASSERT_TRUE(wasTimeSigReceived);
}

void main()
{
  k_msleep(2000);

  UNITY_BEGIN();

  ACTP_init(&ao, ao_dispatch, &qdtest, &tdtest);
  ACTP_start(&ao);

  Signal_init(&testSig, &ao, TEST_SIG);
  Signal_init(&timeSig, &ao, TIME_SIG);
  TimeEvt_init(&timeEvt, &ao, EVT_UPCAST(&timeSig), &ao, NULL);

  RUN_TEST(test_function_active_post);
  RUN_TEST(test_function_active_post_timeevt);

  UNITY_END();
}