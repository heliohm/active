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
  ONESHOT_STATIC_STARTSTOP,
  ONESHOT_DYNAMIC_STARTSTOP,
  ONESHOT_STATIC_EXPIRE,
  ONESHOT_DYNAMIC_EXPIRE,
  ONESHOT_STATIC_EXPIRE_EXPFN_1,
  ONESHOT_STATIC_EXPIRE_EXPFN_2,
  ONESHOT_DYNAMIC_EXPIRE_EXPFN_1,
  ONESHOT_DYNAMIC_EXPIRE_EXPFN_2,
  PERIODIC_STATIC_EXPIRE,
  PERIODIC_DYNAMIC_EXPIRE,
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

static void test_function_timer_dynamic_oneshot_startstop()
{

  const size_t timeOutMs = 20;
  const size_t periodMs = 0;

  size_t sigUsed = Active_mem_Signal_getUsed();
  size_t timeEvtUse = Active_mem_TimeEvt_getUsed();

  expectedSignal = ONESHOT_DYNAMIC_STARTSTOP;

  Signal *sig;
  sig = Signal_new(&ao, ONESHOT_DYNAMIC_STARTSTOP);

  TimeEvt *te;
  te = TimeEvt_new(EVT_UPCAST(sig), &ao, &ao, NULL);

  Active_TimeEvt_start(te, timeOutMs, periodMs);

  /* Test timer flag is indicating to be running */
  TEST_ASSERT_TRUE(te->timer.running);

  /* Test timer indicated is was running when it was stopped */
  bool status = Active_TimeEvt_stop(te);
  TEST_ASSERT_TRUE(status);

  /* Test timer flag is indicating to not be running */
  TEST_ASSERT_FALSE(te->timer.running);

  k_msleep(timeOutMs);

  /* Test timer actually did stop - no event received */
  TEST_ASSERT_EQUAL_UINT16(0, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(0, correctEventsReceived);

  /* Test memory management correctly freeing events when stopping prematurely */
  TEST_ASSERT_EQUAL(sigUsed, Active_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, Active_mem_TimeEvt_getUsed());
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

/* Test normal operation of a one shot timer that expires normally */
static void test_function_timer_dynamic_oneshot_expire()
{
  const size_t timeOutMs = 10;
  const size_t periodMs = 0;

  size_t sigUsed = Active_mem_Signal_getUsed();
  size_t timeEvtUse = Active_mem_TimeEvt_getUsed();

  expectedSignal = ONESHOT_DYNAMIC_EXPIRE;

  Signal *sig = Signal_new(&ao, ONESHOT_DYNAMIC_EXPIRE);

  TimeEvt *te = TimeEvt_new(EVT_UPCAST(sig), &ao, &ao, NULL);

  /* Test timer starting and expiring */
  int64_t timeBeforeTest = k_uptime_get();

  Active_TimeEvt_start(te, timeOutMs, periodMs);

  // Run to expiration
  k_msleep(timeOutMs);

  /* Test timing is correct */
  TEST_ASSERT_EQUAL_INT32(timeOutMs, (int32_t)(timeReceivedMs - timeBeforeTest));

  /* Test timer event posted attached event */
  TEST_ASSERT_EQUAL_UINT16(1, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  /* Test no more events are received => one shot working as intended */
  k_msleep(1 * timeOutMs);

  TEST_ASSERT_EQUAL_INT32(timeOutMs, (int32_t)(timeReceivedMs - timeBeforeTest));
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  /* Test memory management correctly freeing events when stopping prematurely */
  TEST_ASSERT_EQUAL(sigUsed, Active_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, Active_mem_TimeEvt_getUsed());
}

static Event *static_oneshot_expFn(const TimeEvt *const te)
{
  const static SIGNAL_DEFINE(sig_timer_static_expfn_1, ONESHOT_STATIC_EXPIRE_EXPFN_1);

  Signal *s = EVT_CAST(te->e, Signal);

  // Replace event
  if (s->sig == ONESHOT_STATIC_EXPIRE_EXPFN_2)
  {
    return EVT_UPCAST(&sig_timer_static_expfn_1);
  }
  return (Event *)NULL;
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

static Event *dynamic_oneshot_expFn(const TimeEvt *const te)
{

  Signal *s = EVT_CAST(te->e, Signal);

  // Replace event
  if (s->sig == ONESHOT_DYNAMIC_EXPIRE_EXPFN_2)
  {
    return EVT_UPCAST(Signal_new(&ao, ONESHOT_DYNAMIC_EXPIRE_EXPFN_1));
  }
  // Default: Don't replace event
  return (Event *)NULL;
}

/* Test expiry function is working */
static void test_function_timer_dynamic_expFn()
{
  const size_t timeOutMs = 10;
  const size_t periodMs = 0;

  size_t sigUsed = Active_mem_Signal_getUsed();
  size_t timeEvtUse = Active_mem_TimeEvt_getUsed();

  expectedSignal = ONESHOT_DYNAMIC_EXPIRE_EXPFN_1;

  Signal *sig = Signal_new(&ao, ONESHOT_DYNAMIC_EXPIRE_EXPFN_1);

  TimeEvt *te = TimeEvt_new(EVT_UPCAST(sig), &ao, &ao, dynamic_oneshot_expFn);

  Active_TimeEvt_start(te, timeOutMs, periodMs);

  // Run to expiration
  k_msleep(timeOutMs);

  /* Test timer event posted attached event */
  TEST_ASSERT_EQUAL_UINT16(1, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  // Re-initialize attached signal that is now stopped with signal that should be replaced
  sig = Signal_new(&ao, ONESHOT_DYNAMIC_EXPIRE_EXPFN_2);
  te = TimeEvt_new(EVT_UPCAST(sig), &ao, &ao, dynamic_oneshot_expFn);

  Active_TimeEvt_start(te, timeOutMs, periodMs);

  // Run to expiration
  k_msleep(timeOutMs);

  /* Test timer event posted new event from expiry function */
  TEST_ASSERT_EQUAL_UINT16(2, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(2, correctEventsReceived);

  /* Test memory management correctly freeing events when stopping prematurely and expiring */
  TEST_ASSERT_EQUAL(sigUsed, Active_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, Active_mem_TimeEvt_getUsed());
}

static void test_function_timer_static_periodic_expire()
{
  const size_t timeOutMs = 10;
  const size_t periodMs = 10;

  const size_t numEvents = 10;

  expectedSignal = PERIODIC_STATIC_EXPIRE;

  Signal sig;
  Signal_init(&sig, &ao, PERIODIC_STATIC_EXPIRE);

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

static void test_function_timer_dynamic_periodic_expire()
{
  const size_t timeOutMs = 10;
  const size_t periodMs = 10;

  size_t sigUsed = Active_mem_Signal_getUsed();
  size_t timeEvtUse = Active_mem_TimeEvt_getUsed();

  const size_t numEvents = 10;

  expectedSignal = PERIODIC_DYNAMIC_EXPIRE;

  Signal *sig;
  sig = Signal_new(&ao, PERIODIC_DYNAMIC_EXPIRE);

  TimeEvt *te;
  te = TimeEvt_new(EVT_UPCAST(sig), &ao, &ao, NULL);

  Active_TimeEvt_start(te, timeOutMs, periodMs);
  /* Timer is indicating to be running */
  TEST_ASSERT_TRUE(te->timer.running);

  k_msleep(numEvents * periodMs);

  bool status = Active_TimeEvt_stop(te);

  /* Test status returned as running */
  TEST_ASSERT_TRUE(status);

  /* Correct number of events fired */
  TEST_ASSERT_EQUAL_UINT16(numEvents, correctEventsReceived);

  /* Time between two last events is correct */
  TEST_ASSERT_EQUAL_INT32(periodMs, (int32_t)(timeReceivedMs - lastTimeReceivedMs));

  k_msleep(1 * periodMs);

  /* Timer is stopped - no more events are fired */
  TEST_ASSERT_FALSE(te->timer.running);
  TEST_ASSERT_EQUAL_UINT16(numEvents, correctEventsReceived);

  /* Test memory management correctly freeing events after stopping  */
  TEST_ASSERT_EQUAL(sigUsed, Active_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, Active_mem_TimeEvt_getUsed());
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

  /* Test a oneshot timer event w attached event that is started and stopped before expiry */
  RUN_TEST(test_function_timer_static_oneshot_startstop);
  RUN_TEST(test_function_timer_dynamic_oneshot_startstop);

  /* Test a oneshot timer w attached event that is started and runs to expiry */
  RUN_TEST(test_function_timer_static_oneshot_expire);
  RUN_TEST(test_function_timer_dynamic_oneshot_expire);

  /* Test a oneshot static timer w static attached event that uses an expiry function to conditiononally replace attached event */
  RUN_TEST(test_function_timer_static_expFn);
  RUN_TEST(test_function_timer_dynamic_expFn);

  /* Test a periodic timer w attached event and no expiry function ("tick") */
  RUN_TEST(test_function_timer_static_periodic_expire);
  RUN_TEST(test_function_timer_dynamic_periodic_expire);

  UNITY_END();
}
