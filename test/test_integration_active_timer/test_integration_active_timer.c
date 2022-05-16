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
  SIGNAL_UNDEFINED = USER_SIG,
  STATIC_SIG,
  ONESHOT_STATIC_STARTSTOP,
  ONESHOT_STATIC_EXPIRE,
  ONESHOT_STATIC_EXPIRE_EXPFN_1,
  ONESHOT_STATIC_EXPIRE_EXPFN_2,
  PERIODIC_STATIC,
  DYNAMIC_SIG_1,
  DYNAMIC_SIG_2
};

Active ao;

Signal sigStaticOneShot, sigStaticPeriodic, *sigDynamicOneShotPtr, *sigDynamicPeriodicPtr;

TimeEvt timeEvtStaticOneShot, timeEvtStaticPeriodic, *timeEvtDynamicOneShotPtr, *timeEvtDynamicPeriodicPtr;

uint16_t numStaticPeriodicReceived = 0, numDynamicOneShotReceived = 0, numDynamicPeriodicReceived = 0;

uint16_t numExpiryFxnOneShotCalled = 0, numExpiryFxnPeriodicCalled = 0;

int64_t timeReceivedMs = 0, lastTimeReceivedMs = 0;

RefCnt refCntDynamic = 0;

enum TestUserSignal expectedSignal = SIGNAL_UNDEFINED;
uint16_t eventsReceived, correctEventsReceived;
uint16_t expFnNumCalled;

static void ao_dispatch(Active *me, Event const *const e)
{

  // Ignore start signal
  if (e->type == SIGNAL && EVT_CAST(e, Signal)->sig == START_SIG)
  {
    return;
  }

  // Count total events
  eventsReceived++;

  // Ignore events other than signal type in test
  if (e->type != SIGNAL)
  {
    return;
  }

  Signal *s = EVT_CAST(e, Signal);

  // Count number of expected events
  if (expectedSignal == s->sig)
  {

    correctEventsReceived++;
    lastTimeReceivedMs = timeReceivedMs;
    timeReceivedMs = k_uptime_get();
  }

  if ((e->type == SIGNAL) && (EVT_CAST(e, Signal)->sig == DYNAMIC_SIG_1) && (e == EVT_UPCAST(sigDynamicOneShotPtr)))
  {
    numDynamicOneShotReceived++;
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

    sigDynamicPeriodicPtr = Signal_new(&ao, DYNAMIC_SIG_1);

    return EVT_UPCAST(sigDynamicPeriodicPtr);
  }

  //* Keep event */
  return (Event *)NULL;
}

static void test_function_timer_static_oneshot_startstop()
{

  const size_t timeOutMs = 20;
  const size_t periodMs = 0;

  expectedSignal = ONESHOT_STATIC_STARTSTOP;

  Signal sig;
  Signal_init(&sig, &ao, expectedSignal);

  TimeEvt te;
  TimeEvt_init(&te, &ao, EVT_UPCAST(&sig), &ao, NULL);
  Active_TimeEvt_start(&te, timeOutMs, periodMs);

  /* Test timer flag is indicating to be running */
  TEST_ASSERT_TRUE(te.timer.running);

  /* Test timer indicated is was running when it was stopped */
  bool status = Active_TimeEvt_stop(&te);
  TEST_ASSERT_TRUE(status);

  /* Test timer flag is indicating to not be running */
  TEST_ASSERT_FALSE(te.timer.running);

  k_msleep(timeOutMs);

  /* Test timer actually did stop - no event received */
  TEST_ASSERT_EQUAL_UINT16(0, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(0, correctEventsReceived);
}

/* Test normal operation of a one shot timer that expires normally */
static void test_function_timer_static_oneshot_expire()
{

  const size_t timeOutMs = 10;
  const size_t periodMs = 0;

  expectedSignal = ONESHOT_STATIC_EXPIRE;

  Signal sig;
  Signal_init(&sig, &ao, expectedSignal);

  TimeEvt te;
  TimeEvt_init(&te, &ao, EVT_UPCAST(&sig), &ao, NULL);

  /* Test timer starting and expiring */
  int64_t timeBeforeTest = k_uptime_get();

  Active_TimeEvt_start(&te, timeOutMs, periodMs);

  /* Test timer flag is indicating to be running */
  TEST_ASSERT_TRUE(te.timer.running);

  // Run to expiration
  k_msleep(timeOutMs);

  /* Test timing is correct */
  TEST_ASSERT_EQUAL_INT32(timeOutMs, (int32_t)(timeReceivedMs - timeBeforeTest));

  /* Test timer flag is indicating to not longer be running */
  TEST_ASSERT_FALSE(te.timer.running);

  /* Test timer event posted attached event */
  TEST_ASSERT_EQUAL_UINT16(1, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  /* Test no more events are received => one shot working as intended */
  k_msleep(1 * timeOutMs);

  TEST_ASSERT_EQUAL_INT32(timeOutMs, (int32_t)(timeReceivedMs - timeBeforeTest));
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  /* Stopping an expired one shot timer returns that timer is stopped already */
  bool status = Active_TimeEvt_stop(&timeEvtStaticOneShot);
  TEST_ASSERT_FALSE(status);
}

static Event *static_oneshot_expFn(const TimeEvt *const te)
{
  const static SIGNAL_DEFINE(sig_timer_static_expfn_1, ONESHOT_STATIC_EXPIRE_EXPFN_1);

  Signal *s = EVT_CAST(te->e, Signal);

  // Do not replace event
  if (s->sig == ONESHOT_STATIC_EXPIRE_EXPFN_1)
  {
    return NULL;
  }
  // Replace event
  if (s->sig == ONESHOT_STATIC_EXPIRE_EXPFN_2)
  {
    return EVT_UPCAST(&sig_timer_static_expfn_1);
  }
}

/* Test expiry function is working */
static void test_function_timer_static_expFn()
{
  const size_t timeOutMs = 10;
  const size_t periodMs = 0;

  expectedSignal = ONESHOT_STATIC_EXPIRE_EXPFN_1;

  Signal sig;
  Signal_init(&sig, &ao, ONESHOT_STATIC_EXPIRE_EXPFN_1);

  TimeEvt te;
  TimeEvt_init(&te, &ao, EVT_UPCAST(&sig), &ao, static_oneshot_expFn);

  Active_TimeEvt_start(&te, timeOutMs, periodMs);

  // Run to expiration
  k_msleep(timeOutMs);

  /* Test timer event posted attached event */
  TEST_ASSERT_EQUAL_UINT16(1, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  // Re-initialize attached signal that is now stopped with signal that should be replaced
  Signal_init(&sig, &ao, ONESHOT_STATIC_EXPIRE_EXPFN_2);

  Active_TimeEvt_start(&te, timeOutMs, periodMs);

  // Run to expiration
  k_msleep(timeOutMs);

  /* Test timer event posted new event from expiry function */
  TEST_ASSERT_EQUAL_UINT16(2, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(2, correctEventsReceived);
}

static void test_function_active_timer_static_periodic()
{
  const size_t timeOutMs = 10;
  const size_t periodMs = 10;

  const size_t numEvents = 10;

  expectedSignal = PERIODIC_STATIC;

  Signal sig;
  Signal_init(&sig, &ao, PERIODIC_STATIC);

  TimeEvt te;
  TimeEvt_init(&te, &ao, EVT_UPCAST(&sig), &ao, NULL);

  Active_TimeEvt_start(&te, timeOutMs, periodMs);
  /* Timer is indicating to be running */
  TEST_ASSERT_TRUE(te.timer.running);

  k_msleep(numEvents * periodMs);

  bool status = Active_TimeEvt_stop(&te);

  /* Test status returned as running */
  TEST_ASSERT_TRUE(status);

  /* Correct number of events fired */
  TEST_ASSERT_EQUAL_UINT16(numEvents, correctEventsReceived);

  /* Time between two last events is correct */
  TEST_ASSERT_EQUAL_INT32(periodMs, (int32_t)(timeReceivedMs - lastTimeReceivedMs));

  k_msleep(1 * periodMs);

  /* Timer is stopped - no more events are fired */
  TEST_ASSERT_FALSE(te.timer.running);
  TEST_ASSERT_EQUAL_UINT16(numEvents, correctEventsReceived);
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
  k_msleep(2 * timeOutMs);

  TEST_ASSERT_EQUAL(timeEvtUse, Active_mem_TimeEvt_getUsed());
  TEST_ASSERT_EQUAL(sigUsed, Active_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL_UINT16(1, numDynamicOneShotReceived);

  /* Test expiry function was called */
  TEST_ASSERT_EQUAL_UINT16(1, numExpiryFxnOneShotCalled);

  /* Test no more events received */
  k_msleep(1 * timeOutMs);
  TEST_ASSERT_EQUAL_UINT16(1, numDynamicOneShotReceived);

  /* Test automatic memory management of event */
  TEST_ASSERT_EQUAL(1, refCntDynamic);
  TEST_ASSERT_EQUAL(0, Active_mem_getRefCount(timeEvtDynamicOneShotPtr->e));
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

  Active_TimeEvt_stop(timeEvtDynamicPeriodicPtr);
  TEST_ASSERT_EQUAL_UINT16(numEvents, numDynamicPeriodicReceived);

  /* Test memory management */
  TEST_ASSERT_EQUAL_UINT32(sigUsed, Active_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL_UINT32(timeEvtUse, Active_mem_TimeEvt_getUsed());
}

void setUp()
{
  numStaticPeriodicReceived = 0, numDynamicOneShotReceived = 0, numDynamicPeriodicReceived = 0;

  numExpiryFxnOneShotCalled = 0, numExpiryFxnPeriodicCalled = 0;

  refCntDynamic = 0;

  expectedSignal = SIGNAL_UNDEFINED;
  eventsReceived = 0;
  correctEventsReceived = 0;
  expFnNumCalled = 0;

  timeReceivedMs = 0, lastTimeReceivedMs = 0;
}

void main()
{
  k_msleep(2000);

  UNITY_BEGIN();

  Active_init(&ao, ao_dispatch);
  Active_start(&ao, &qdtest, &tdtest);

  /* Test a static timer w static attached event that is started and stopped before expiry */
  RUN_TEST(test_function_timer_static_oneshot_startstop);
  /* Test a static timer w static attached event that is started and runs to expiry */
  RUN_TEST(test_function_timer_static_oneshot_expire);
  /* Test a static timer w static attached event that uses an expiry function to replace attached event or not */
  RUN_TEST(test_function_timer_static_expFn);
  // RUN_TEST(test_function_active_timer_static_oneshot);
  RUN_TEST(test_function_active_timer_static_periodic);

  // RUN_TEST(test_function_active_timer_dynamic_periodic);
  // RUN_TEST(test_function_active_timer_dynamic_oneshot);

  //
  UNITY_END();
}
