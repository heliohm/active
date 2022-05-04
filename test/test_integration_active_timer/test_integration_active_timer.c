#include <active.h>
#include <unity.h>

static ACTIVE_QBUF(testQBuf, 1);
static ACTIVE_Q(testQ);
static ACTIVE_THREAD(testT);
static ACTIVE_THREAD_STACK(testTStack, 512);
static ACTIVE_THREAD_STACK_SIZE(testTStackSz, testTStack);

const static queueData qdtest = {.maxMsg = 1,
                                 .queBuf = testQBuf,
                                 .queue = &testQ};

const static threadData tdtest = {.thread = &testT,
                                  .pri = 1,
                                  .stack = testTStack,
                                  .stack_size = testTStackSz};

enum TestUserSignal
{
  STATIC_SIG = USER_SIG,
  DYNAMIC_SIG_1,
  DYNAMIC_SIG_2
};

Active ao;

Signal sigStaticOneShot, sigStaticPeriodic, *sigDynamicOneShotPtr, *sigDynamicPeriodicPtr;

TimeEvt timeEvtStaticOneShot, timeEvtStaticPeriodic, *timeEvtDynamicOneShotPtr, *timeEvtDynamicPeriodicPtr;

uint16_t numStaticOneShotReceived = 0, numStaticPeriodicReceived = 0, numDynamicOneShotReceived = 0, numDynamicPeriodicReceived = 0;

uint16_t numExpiryFxnOneShotCalled = 0, numExpiryFxnPeriodicCalled = 0;

int64_t timeReceivedMs = 0, lastTimeReceivedMs = 0;

RefCnt refCntDynamic = 0;

static void ao_dispatch(Active *me, Event const *const e)
{

  if ((e->type == SIGNAL) && (EVT_CAST(e, Signal)->sig == STATIC_SIG) && (e == EVT_UPCAST(&sigStaticOneShot)))
  {
    numStaticOneShotReceived++;

    timeReceivedMs = k_uptime_get();
  }

  if ((e->type == SIGNAL) && (EVT_CAST(e, Signal)->sig == STATIC_SIG) && (e == EVT_UPCAST(&sigStaticPeriodic)))
  {
    numStaticPeriodicReceived++;

    lastTimeReceivedMs = timeReceivedMs;
    timeReceivedMs = k_uptime_get();
  }

  if ((e->type == SIGNAL) && (EVT_CAST(e, Signal)->sig == DYNAMIC_SIG_1) && (e == EVT_UPCAST(sigDynamicOneShotPtr)))
  {
    numDynamicOneShotReceived++;
    refCntDynamic = Active_mem_getRefCount(e);
  }

  if ((e->type == SIGNAL) && ((EVT_CAST(e, Signal)->sig == DYNAMIC_SIG_1) || (EVT_CAST(e, Signal)->sig == DYNAMIC_SIG_2)) && (e == EVT_UPCAST(sigDynamicPeriodicPtr)))
  {
    numDynamicPeriodicReceived++;
    refCntDynamic = Active_mem_getRefCount(e);
  }
}

static Event *expiryFn(const TimeEvt *const te)
{
  /* One shot time event */
  if (te == timeEvtDynamicOneShotPtr)
  {
    numExpiryFxnOneShotCalled++;
    return EVT_UPCAST(sigDynamicOneShotPtr);
  }
  /* Periodic time event */
  else if (te == timeEvtDynamicPeriodicPtr)
  {
    // Return early - Event already allocated in test
    if (numExpiryFxnPeriodicCalled == 0)
    {
      numExpiryFxnPeriodicCalled++;
      return EVT_UPCAST(sigDynamicPeriodicPtr);
    }

    // Alternate between signals
    if (EVT_CAST(te->e, Signal)->sig == DYNAMIC_SIG_1)
    {
      sigDynamicPeriodicPtr = Signal_new(&ao, DYNAMIC_SIG_2);
    }
    else
    {
      sigDynamicPeriodicPtr = Signal_new(&ao, DYNAMIC_SIG_1);
    }

    return EVT_UPCAST(sigDynamicPeriodicPtr);
  }

  /* Error case */
  return (Event *)NULL;
}

static void test_function_active_timer_static_oneshot()
{
  const size_t timeOutMs = 20;

  Signal_init(&sigStaticOneShot, &ao, STATIC_SIG);
  TimeEvt_init(&timeEvtStaticOneShot, &ao, EVT_UPCAST(&sigStaticOneShot), &ao, NULL);

  Active_TimeEvt_start(&timeEvtStaticOneShot, timeOutMs, 0);
  /* Timer is indicating to be running */
  TEST_ASSERT_TRUE(timeEvtStaticOneShot.timer.running);

  /* Timer indicated is was running when it was stopped */
  bool status = Active_TimeEvt_stop(&timeEvtStaticOneShot);
  TEST_ASSERT_TRUE(status);

  k_msleep(timeOutMs);
  /* Timer actually did stop - no event received */
  TEST_ASSERT_EQUAL_UINT16(0, numStaticOneShotReceived);

  /* Test timer starting and expiring */
  int64_t timeBeforeTest = k_uptime_get();
  Active_TimeEvt_start(&timeEvtStaticOneShot, timeOutMs, 0);
  k_msleep(timeOutMs);

  /* Event is received */
  TEST_ASSERT_EQUAL_UINT16(1, numStaticOneShotReceived);
  /* Timing is correct */
  TEST_ASSERT_EQUAL_INT32(timeOutMs, (int32_t)(timeReceivedMs - timeBeforeTest));
  /* Timer is indicated to be stopped */
  TEST_ASSERT_FALSE(timeEvtStaticOneShot.timer.running);

  /* No more events are received => one shot working as intended*/
  k_msleep(1 * timeOutMs);
  TEST_ASSERT_EQUAL_INT32(timeOutMs, (int32_t)(timeReceivedMs - timeBeforeTest));

  /* Stopping an expired one shot timer returns that timer is stopped already */
  status = Active_TimeEvt_stop(&timeEvtStaticOneShot);
  TEST_ASSERT_FALSE(status);
}

static void test_function_active_timer_static_periodic()
{
  static const size_t periodMs = 10;
  static const size_t numEvents = 3;

  Signal_init(&sigStaticPeriodic, &ao, STATIC_SIG);
  TimeEvt_init(&timeEvtStaticPeriodic, &ao, EVT_UPCAST(&sigStaticPeriodic), &ao, NULL);

  Active_TimeEvt_start(&timeEvtStaticPeriodic, periodMs, periodMs);
  /* Timer is indicating to be running */
  TEST_ASSERT_TRUE(timeEvtStaticPeriodic.timer.running);

  k_msleep(numEvents * periodMs);
  Active_TimeEvt_stop(&timeEvtStaticPeriodic);

  /* Correct number of events fired */
  TEST_ASSERT_EQUAL_UINT16(numEvents, numStaticPeriodicReceived);
  /* Time between two last events is correct */
  TEST_ASSERT_EQUAL_INT32(periodMs, (int32_t)(timeReceivedMs - lastTimeReceivedMs));

  k_msleep(1 * periodMs);

  /* Timer is stopped - no more events are fired */
  TEST_ASSERT_FALSE(timeEvtStaticPeriodic.timer.running);
  TEST_ASSERT_EQUAL_UINT16(numEvents, numStaticPeriodicReceived);
}

static void test_function_active_timer_dynamic_oneshot()
{
  static const size_t timeOutMs = 10;

  size_t sigUsed = Active_mem_Signal_getUsed();
  size_t timeEvtUse = Active_mem_TimeEvt_getUsed();

  sigDynamicOneShotPtr = Signal_new(&ao, DYNAMIC_SIG_1);
  timeEvtDynamicOneShotPtr = TimeEvt_new(EVT_UPCAST(sigDynamicOneShotPtr), &ao, &ao, expiryFn);

  /* Test start and stop */
  Active_TimeEvt_start(timeEvtDynamicOneShotPtr, timeOutMs, 0);
  /* Test internal flag for running */
  TEST_ASSERT_TRUE(timeEvtDynamicOneShotPtr->timer.running);

  /* Test that timer indicated it was running when stopped */
  bool status = Active_TimeEvt_stop(timeEvtDynamicOneShotPtr);
  TEST_ASSERT_TRUE(status);

  /* Test internal flag for running */
  TEST_ASSERT_FALSE(timeEvtDynamicOneShotPtr->timer.running);

  /* Test event is not received yet*/
  k_msleep(timeOutMs);
  TEST_ASSERT_EQUAL_UINT16(0, numDynamicOneShotReceived);

  /* Test memory management correctly freeing events when stopping prematurely */
  TEST_ASSERT_EQUAL(sigUsed, Active_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, Active_mem_TimeEvt_getUsed());

  /* Create new timer event and new attached event and let expire */
  sigDynamicOneShotPtr = Signal_new(&ao, DYNAMIC_SIG_1);
  timeEvtDynamicOneShotPtr = TimeEvt_new(EVT_UPCAST(sigDynamicOneShotPtr), &ao, &ao, expiryFn);
  Active_TimeEvt_start(timeEvtDynamicOneShotPtr, timeOutMs, 0);

  /* Test event is actually received */
  k_msleep(timeOutMs);
  TEST_ASSERT_EQUAL_UINT16(1, numDynamicOneShotReceived);

  /* Test expiry function was called */
  TEST_ASSERT_EQUAL_UINT16(1, numExpiryFxnOneShotCalled);

  /* Test no more events received */
  k_msleep(1 * timeOutMs);
  TEST_ASSERT_EQUAL_UINT16(1, numDynamicOneShotReceived);

  /* Test automatic memory management of event */
  //  TEST_ASSERT_EQUAL(1, refCntDynamic);
  // TEST_ASSERT_EQUAL(0, Active_mem_getRefCount(timeEvtDynamicOneShotPtr->e));

  TEST_ASSERT_EQUAL(sigUsed, Active_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, Active_mem_TimeEvt_getUsed());
}

static void test_function_active_timer_dynamic_periodic()
{
  static const size_t periodMs = 100;
  static const size_t numEvents = 10;

  uint32_t sigUsed = Active_mem_Signal_getUsed();
  uint32_t timeEvtUse = Active_mem_TimeEvt_getUsed();

  sigDynamicPeriodicPtr = Signal_new(&ao, DYNAMIC_SIG_1);
  timeEvtDynamicPeriodicPtr = TimeEvt_new(EVT_UPCAST(sigDynamicPeriodicPtr), &ao, &ao, expiryFn);

  Active_TimeEvt_start(timeEvtDynamicPeriodicPtr, periodMs, periodMs);

  /* Test event is actually received */
  k_msleep(periodMs);
  TEST_ASSERT_EQUAL_UINT16(1, numDynamicPeriodicReceived);

  /* Test that remaining events are received */
  k_msleep((numEvents - 1) * periodMs);

  TEST_ASSERT_EQUAL_UINT32(sigUsed, Active_mem_Signal_getUsed());

  Active_TimeEvt_stop(timeEvtDynamicPeriodicPtr);
  TEST_ASSERT_EQUAL_UINT16(numEvents, numDynamicPeriodicReceived);

  /* Test memory management */
  TEST_ASSERT_EQUAL_UINT32(sigUsed, Active_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL_UINT32(timeEvtUse, Active_mem_TimeEvt_getUsed());
}

void setUp()
{
  numStaticOneShotReceived = 0, numStaticPeriodicReceived = 0, numDynamicOneShotReceived = 0, numDynamicPeriodicReceived = 0;

  numExpiryFxnOneShotCalled = 0, numExpiryFxnPeriodicCalled = 0;

  timeReceivedMs = 0, lastTimeReceivedMs = 0;

  refCntDynamic = 0;
}

void main()
{
  k_msleep(2000);

  UNITY_BEGIN();

  Active_init(&ao, ao_dispatch);
  Active_start(&ao, &qdtest, &tdtest);
  RUN_TEST(test_function_active_timer_static_oneshot);
  RUN_TEST(test_function_active_timer_dynamic_periodic);

  RUN_TEST(test_function_active_timer_static_periodic);
  RUN_TEST(test_function_active_timer_dynamic_oneshot);

  UNITY_END();
}
