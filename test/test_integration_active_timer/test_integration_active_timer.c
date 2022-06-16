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
  ACT_SIGNAL_UNDEFINED = ACT_USER_SIG,
  ONESHOT_STATIC_STARTSTOP,
  ONESHOT_DYNAMIC_STARTSTOP,
  ONESHOT_STATIC_EXPIRE,
  ONESHOT_DYNAMIC_EXPIRE,
  ONESHOT_DYNAMIC_EXPIRE_STOP,
  ONESHOT_STATIC_EXPIRE_EXPFN_1,
  ONESHOT_STATIC_EXPIRE_EXPFN_2,
  ONESHOT_DYNAMIC_EXPIRE_EXPFN_1,
  ONESHOT_DYNAMIC_EXPIRE_EXPFN_2,
  PERIODIC_STATIC_EXPIRE,
  PERIODIC_DYNAMIC_EXPIRE
};

Active ao;

int64_t timeReceivedMs = 0, lastTimeReceivedMs = 0;
enum TestUserSignal expectedSignal = ACT_SIGNAL_UNDEFINED;
uint16_t eventsReceived, correctEventsReceived;
uint32_t expectedMsgPayload = 0;

static void ao_dispatch(Active *me, ACT_Evt const *const e)
{

  // Ignore start signal
  if (e->type == ACT_SIGNAL && EVT_CAST(e, ACT_Signal)->sig == ACT_START_SIG)
  {
    return;
  }

  // Count total events
  eventsReceived++;

  // Ignore events other than signal type in test
  if (e->type == ACT_SIGNAL)
  {
    ACT_Signal *s = EVT_CAST(e, ACT_Signal);

    // Count number of expected events
    if (expectedSignal == s->sig)
    {

      correctEventsReceived++;
      lastTimeReceivedMs = timeReceivedMs;
      timeReceivedMs = ACT_TIMEMS_GET();
    }
  }
  else if (e->type == ACT_MESSAGE)
  {
    ACT_Message *m = EVT_CAST(e, ACT_Message);
    if (m->header == 0xBABA && expectedMsgPayload == *(uint32_t *)m->payload)
    {
      correctEventsReceived++;
    }
  }
}

static void test_function_timer_static_oneshot_startstop()
{

  const size_t timeOutMs = 20;
  const size_t periodMs = 0;

  expectedSignal = ONESHOT_STATIC_STARTSTOP;

  ACT_Signal sig;
  ACT_Signal_init(&sig, &ao, expectedSignal);

  ACT_TimEvt te;
  ACT_TimEvt_init(&te, &ao, EVT_UPCAST(&sig), &ao, NULL);
  ACT_TimeEvt_start(&te, timeOutMs, periodMs);

  /* Test timer flag is indicating to be running */
  TEST_ASSERT_TRUE(te.timer.running);

  /* Test timer indicated is was running when it was stopped */
  bool status = ACT_TimeEvt_stop(&te);
  TEST_ASSERT_TRUE(status);

  /* Test timer flag is indicating to not be running */
  TEST_ASSERT_FALSE(te.timer.running);

  ACT_SLEEPMS(timeOutMs);

  /* Test timer actually did stop - no event received */
  TEST_ASSERT_EQUAL_UINT16(0, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(0, correctEventsReceived);
}

static void test_function_timer_dynamic_oneshot_startstop()
{

  const size_t timeOutMs = 20;
  const size_t periodMs = 0;

  size_t sigUsed = ACT_mem_Signal_getUsed();
  size_t timeEvtUse = ACT_mem_TimeEvt_getUsed();

  expectedSignal = ONESHOT_DYNAMIC_STARTSTOP;

  ACT_Signal *sig;
  sig = ACT_Signal_new(&ao, ONESHOT_DYNAMIC_STARTSTOP);

  ACT_TimEvt *te;
  te = ACT_TimEvt_new(EVT_UPCAST(sig), &ao, &ao, NULL);

  ACT_TimeEvt_start(te, timeOutMs, periodMs);

  /* Test timer flag is indicating to be running */
  TEST_ASSERT_TRUE(te->timer.running);

  /* Test timer indicated is was running when it was stopped */
  bool status = ACT_TimeEvt_stop(te);
  TEST_ASSERT_TRUE(status);

  /* Test timer flag is indicating to not be running */
  TEST_ASSERT_FALSE(te->timer.running);

  ACT_SLEEPMS(timeOutMs);

  /* Test timer actually did stop - no event received */
  TEST_ASSERT_EQUAL_UINT16(0, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(0, correctEventsReceived);

  /* Test memory management correctly freeing events when stopping prematurely */
  TEST_ASSERT_EQUAL(sigUsed, ACT_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, ACT_mem_TimeEvt_getUsed());
}

/* Test normal operation of a one shot timer that expires normally */
static void test_function_timer_static_oneshot_expire()
{

  const size_t timeOutMs = 10;
  const size_t periodMs = 0;

  expectedSignal = ONESHOT_STATIC_EXPIRE;

  ACT_Signal sig;
  ACT_Signal_init(&sig, &ao, expectedSignal);

  ACT_TimEvt te;
  ACT_TimEvt_init(&te, &ao, EVT_UPCAST(&sig), &ao, NULL);

  /* Test timer starting and expiring */
  int64_t timeBeforeTest = ACT_TIMEMS_GET();

  ACT_TimeEvt_start(&te, timeOutMs, periodMs);

  /* Test timer flag is indicating to be running */
  TEST_ASSERT_TRUE(te.timer.running);

  // Run to expiration
  ACT_SLEEPMS(timeOutMs);

  /* Test timing is correct */
  TEST_ASSERT_EQUAL_INT32(timeOutMs, (int32_t)(timeReceivedMs - timeBeforeTest));

  /* Test timer flag is indicating to not longer be running */
  TEST_ASSERT_FALSE(te.timer.running);

  /* Test timer event posted attached event */
  TEST_ASSERT_EQUAL_UINT16(1, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  /* Test no more events are received => one shot working as intended */
  ACT_SLEEPMS(1 * timeOutMs);

  TEST_ASSERT_EQUAL_INT32(timeOutMs, (int32_t)(timeReceivedMs - timeBeforeTest));
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  /* Stopping an expired one shot timer returns that timer is stopped already */
  bool status = ACT_TimeEvt_stop(&te);
  TEST_ASSERT_FALSE(status);
}

/* Test normal operation of a one shot timer that expires normally */
static void test_function_timer_dynamic_oneshot_expire()
{
  const size_t timeOutMs = 10;
  const size_t periodMs = 0;

  size_t sigUsed = ACT_mem_Signal_getUsed();
  size_t timeEvtUse = ACT_mem_TimeEvt_getUsed();

  expectedSignal = ONESHOT_DYNAMIC_EXPIRE;

  ACT_Signal *sig = ACT_Signal_new(&ao, ONESHOT_DYNAMIC_EXPIRE);

  ACT_TimEvt *te = ACT_TimEvt_new(EVT_UPCAST(sig), &ao, &ao, NULL);

  /* Test timer starting and expiring */
  int64_t timeBeforeTest = ACT_TIMEMS_GET();

  ACT_TimeEvt_start(te, timeOutMs, periodMs);

  // Run to expiration
  ACT_SLEEPMS(timeOutMs);

  /* Test timing is correct */
  TEST_ASSERT_EQUAL_INT32(timeOutMs, (int32_t)(timeReceivedMs - timeBeforeTest));

  /* Test timer event posted attached event */
  TEST_ASSERT_EQUAL_UINT16(1, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  /* Test no more events are received => one shot working as intended */
  ACT_SLEEPMS(1 * timeOutMs);

  TEST_ASSERT_EQUAL_INT32(timeOutMs, (int32_t)(timeReceivedMs - timeBeforeTest));
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  /* Test memory management correctly freeing events when stopping prematurely */
  TEST_ASSERT_EQUAL(sigUsed, ACT_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, ACT_mem_TimeEvt_getUsed());
}

// Test checking if dynamic time event has expired before stopping
static void test_function_timer_dynamic_oneshot_expire_stop()
{
  const size_t timeOutMs = 10;
  const size_t sleepMs = 9;
  const size_t periodMs = 0;

  size_t sigUsed = ACT_mem_Signal_getUsed();
  size_t timeEvtUse = ACT_mem_TimeEvt_getUsed();

  expectedSignal = ONESHOT_DYNAMIC_EXPIRE_STOP;

  ACT_Signal *sig;
  ACT_TimEvt *te;

  size_t loops = 0;
  while (1)
  {
    sig = ACT_Signal_new(&ao, expectedSignal);
    te = ACT_TimEvt_new(EVT_UPCAST(sig), &ao, &ao, NULL);
    // Add manual reference on time event to prevent it from being freed when expiring
    ACT_mem_refinc(EVT_UPCAST(te));

    ACT_TimeEvt_start(te, timeOutMs, periodMs);

    // Linearly increasing sleep+busy wait period until hitting timer expiry (potential race condition)
    ACT_SLEEPMS(sleepMs);
    loops++;
    atomic_t i = loops;
    while (atomic_dec(&i))
    {
      __ASM("NOP");
    }

    bool status = ACT_TimeEvt_stop(te);
    ACT_mem_refdec(EVT_UPCAST(te));

    if (status)
    {
      break;
    }
  }
  /* Test memory management correctly freeing events when stopping prematurely */
  TEST_ASSERT_EQUAL(sigUsed, ACT_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, ACT_mem_TimeEvt_getUsed());
}

static ACT_Evt *static_oneshot_expFn(const ACT_TimEvt *const te)
{
  const static ACT_SIGNAL_DEFINE(sig_timer_static_expfn_1, ONESHOT_STATIC_EXPIRE_EXPFN_1);

  ACT_Signal *s = EVT_CAST(te->e, ACT_Signal);

  // Replace event
  if (s->sig == ONESHOT_STATIC_EXPIRE_EXPFN_2)
  {
    return EVT_UPCAST(&sig_timer_static_expfn_1);
  }
  return (ACT_Evt *)NULL;
}

/* Test expiry function is working */
static void test_function_timer_static_expFn()
{
  const size_t timeOutMs = 10;
  const size_t periodMs = 0;

  expectedSignal = ONESHOT_STATIC_EXPIRE_EXPFN_1;

  ACT_Signal sig;
  ACT_Signal_init(&sig, &ao, ONESHOT_STATIC_EXPIRE_EXPFN_1);

  ACT_TimEvt te;
  ACT_TimEvt_init(&te, &ao, EVT_UPCAST(&sig), &ao, static_oneshot_expFn);

  ACT_TimeEvt_start(&te, timeOutMs, periodMs);

  // Run to expiration
  ACT_SLEEPMS(timeOutMs);

  /* Test timer event posted attached event */
  TEST_ASSERT_EQUAL_UINT16(1, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  // Re-initialize attached signal that is now stopped with signal that should be replaced
  ACT_Signal_init(&sig, &ao, ONESHOT_STATIC_EXPIRE_EXPFN_2);

  ACT_TimeEvt_start(&te, timeOutMs, periodMs);

  // Run to expiration
  ACT_SLEEPMS(timeOutMs);

  /* Test timer event posted new event from expiry function */
  TEST_ASSERT_EQUAL_UINT16(2, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(2, correctEventsReceived);
}

static ACT_Evt *dynamic_oneshot_expFn(const ACT_TimEvt *const te)
{

  ACT_Signal *s = EVT_CAST(te->e, ACT_Signal);

  // Replace event
  if (s->sig == ONESHOT_DYNAMIC_EXPIRE_EXPFN_2)
  {
    return EVT_UPCAST(ACT_Signal_new(&ao, ONESHOT_DYNAMIC_EXPIRE_EXPFN_1));
  }
  // Default: Don't replace event
  return (ACT_Evt *)NULL;
}

/* Test expiry function is working */
static void test_function_timer_dynamic_expFn()
{
  const size_t timeOutMs = 10;
  const size_t periodMs = 0;

  size_t sigUsed = ACT_mem_Signal_getUsed();
  size_t timeEvtUse = ACT_mem_TimeEvt_getUsed();

  expectedSignal = ONESHOT_DYNAMIC_EXPIRE_EXPFN_1;

  ACT_Signal *sig = ACT_Signal_new(&ao, ONESHOT_DYNAMIC_EXPIRE_EXPFN_1);

  ACT_TimEvt *te = ACT_TimEvt_new(EVT_UPCAST(sig), &ao, &ao, dynamic_oneshot_expFn);

  ACT_TimeEvt_start(te, timeOutMs, periodMs);

  // Run to expiration
  ACT_SLEEPMS(timeOutMs);

  /* Test timer event posted attached event */
  TEST_ASSERT_EQUAL_UINT16(1, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(1, correctEventsReceived);

  // Re-initialize attached signal that is now stopped with signal that should be replaced
  sig = ACT_Signal_new(&ao, ONESHOT_DYNAMIC_EXPIRE_EXPFN_2);
  te = ACT_TimEvt_new(EVT_UPCAST(sig), &ao, &ao, dynamic_oneshot_expFn);

  ACT_TimeEvt_start(te, timeOutMs, periodMs);

  // Run to expiration
  ACT_SLEEPMS(timeOutMs);

  /* Test timer event posted new event from expiry function */
  TEST_ASSERT_EQUAL_UINT16(2, eventsReceived);
  TEST_ASSERT_EQUAL_UINT16(2, correctEventsReceived);

  /* Test memory management correctly freeing events when stopping prematurely and expiring */
  TEST_ASSERT_EQUAL(sigUsed, ACT_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, ACT_mem_TimeEvt_getUsed());
}

static void test_function_timer_static_periodic_expire()
{
  const size_t timeOutMs = 10;
  const size_t periodMs = 10;

  const size_t numEvents = 10;

  expectedSignal = PERIODIC_STATIC_EXPIRE;

  ACT_Signal sig;
  ACT_Signal_init(&sig, &ao, PERIODIC_STATIC_EXPIRE);

  ACT_TimEvt te;
  ACT_TimEvt_init(&te, &ao, EVT_UPCAST(&sig), &ao, NULL);

  ACT_TimeEvt_start(&te, timeOutMs, periodMs);
  /* Timer is indicating to be running */
  TEST_ASSERT_TRUE(te.timer.running);

  ACT_SLEEPMS(numEvents * periodMs);

  bool status = ACT_TimeEvt_stop(&te);

  /* Test status returned as running */
  TEST_ASSERT_TRUE(status);

  /* Correct number of events fired */
  TEST_ASSERT_EQUAL_UINT16(numEvents, correctEventsReceived);

  /* Time between two last events is correct */
  TEST_ASSERT_EQUAL_INT32(periodMs, (int32_t)(timeReceivedMs - lastTimeReceivedMs));

  ACT_SLEEPMS(1 * periodMs);

  /* Timer is stopped - no more events are fired */
  TEST_ASSERT_FALSE(te.timer.running);
  TEST_ASSERT_EQUAL_UINT16(numEvents, correctEventsReceived);
}

static void test_function_timer_dynamic_periodic_expire()
{
  const size_t timeOutMs = 10;
  const size_t periodMs = 10;

  size_t sigUsed = ACT_mem_Signal_getUsed();
  size_t timeEvtUse = ACT_mem_TimeEvt_getUsed();

  const size_t numEvents = 10;

  expectedSignal = PERIODIC_DYNAMIC_EXPIRE;

  ACT_Signal *sig;
  sig = ACT_Signal_new(&ao, PERIODIC_DYNAMIC_EXPIRE);

  ACT_TimEvt *te;
  te = ACT_TimEvt_new(EVT_UPCAST(sig), &ao, &ao, NULL);

  ACT_TimeEvt_start(te, timeOutMs, periodMs);
  /* Timer is indicating to be running */
  TEST_ASSERT_TRUE(te->timer.running);

  ACT_SLEEPMS(numEvents * periodMs);

  bool status = ACT_TimeEvt_stop(te);

  /* Test status returned as running */
  TEST_ASSERT_TRUE(status);

  /* Correct number of events fired */
  TEST_ASSERT_EQUAL_UINT16(numEvents, correctEventsReceived);

  /* Time between two last events is correct */
  TEST_ASSERT_EQUAL_INT32(periodMs, (int32_t)(timeReceivedMs - lastTimeReceivedMs));

  ACT_SLEEPMS(1 * periodMs);

  /* Timer is stopped - no more events are fired */
  TEST_ASSERT_FALSE(te->timer.running);
  TEST_ASSERT_EQUAL_UINT16(numEvents, correctEventsReceived);

  /* Test memory management correctly freeing events after stopping  */
  TEST_ASSERT_EQUAL(sigUsed, ACT_mem_Signal_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, ACT_mem_TimeEvt_getUsed());
}

static uint32_t buf0[] = {0xDEADBEEF, 0xDEADBEEF};
static uint32_t buf1[] = {0xBADDCAFE, 0xBAAAAAAD};
static const char bufSz = 2;
static uint16_t header = 0xBABA;
static uint32_t *writeBuf = buf0;
static char writeBufIdx = 0;

ACT_Evt *msgExpFn(const ACT_TimEvt *const te)
{

  static ACT_Message *msg = NULL;

  if (msg)
  {
    /* Test same event returned last time called is given in expiry function */
    TEST_ASSERT_EQUAL_PTR(msg, te->e);
  }
  // Prepare message with old write buffer
  msg = ACT_Message_new(&ao, header, writeBuf, bufSz);

  // Swap write buffer.
  writeBuf = (writeBuf == buf0) ? buf1 : buf0;
  writeBufIdx = 0;

  // Return message for updating attached event and posting to receiver
  return EVT_UPCAST(msg);
}

/* Test a periodic dynamic time evt that updates attached message event every expiration.
This is typical for a producer / consumer setting where the consumer is the dispatch function and the
producter is the test function */
static void test_function_timer_dynamic_periodic_expire_expFn()
{

  const size_t timeOutMs = 10;
  const size_t periodMs = 10;

  size_t msgUsed = ACT_mem_Message_getUsed();
  size_t timeEvtUse = ACT_mem_TimeEvt_getUsed();

  size_t numEvents = 10;

  // Initial time event has no attached event.
  ACT_TimEvt *te;
  te = ACT_TimEvt_new(EVT_UPCAST(NULL), &ao, &ao, msgExpFn);

  ACT_TimeEvt_start(te, timeOutMs, periodMs);
  /* Timer is indicating to be running */
  TEST_ASSERT_TRUE(te->timer.running);
  size_t i = numEvents;
  while (i--)
  {
    /* Produce things in buffer
    .
    .
    .
    */

    /* NB: This test is not thread safe, as expiry function is called in ao (active object) context (thread) while
    test function runs in different context.
    However, an active object creating a time event / message will have expiry function called in same context and be safe. */
    expectedMsgPayload = (writeBuf == buf0 ? buf0[0] : buf1[0]);
    ACT_SLEEPMS(periodMs);
  }

  bool status = ACT_TimeEvt_stop(te);

  /* Test status returned as running */
  TEST_ASSERT_TRUE(status);

  /* Correct number of events fired */
  TEST_ASSERT_EQUAL_UINT16(numEvents, correctEventsReceived);

  ACT_SLEEPMS(1 * periodMs);

  /* Timer is stopped - no more events are fired */
  TEST_ASSERT_FALSE(te->timer.running);
  TEST_ASSERT_EQUAL_UINT16(numEvents, correctEventsReceived);

  /* Test memory management correctly freeing events after stopping  */
  TEST_ASSERT_EQUAL(msgUsed, ACT_mem_Message_getUsed());
  TEST_ASSERT_EQUAL(timeEvtUse, ACT_mem_TimeEvt_getUsed());
}

void setUp()
{
  expectedSignal = ACT_SIGNAL_UNDEFINED;
  expectedMsgPayload = 0;
  eventsReceived = 0;
  correctEventsReceived = 0;

  timeReceivedMs = 0, lastTimeReceivedMs = 0;
}

void main()
{
  ACT_SLEEPMS(2000);

  UNITY_BEGIN();

  ACT_init(&ao, ao_dispatch, &qdtest, &tdtest);
  ACT_start(&ao);

  /* Test a oneshot timer event w attached event that is started and stopped before expiry */
  RUN_TEST(test_function_timer_static_oneshot_startstop);
  RUN_TEST(test_function_timer_dynamic_oneshot_startstop);
  /* Test stopping a dynamic oneshot timer around expiration (race condition) */
  RUN_TEST(test_function_timer_dynamic_oneshot_expire_stop);

  /* Test a oneshot timer w attached event that is started and runs to expiry */
  RUN_TEST(test_function_timer_static_oneshot_expire);
  RUN_TEST(test_function_timer_dynamic_oneshot_expire);

  /* Test a oneshot static timer w static attached event that uses an expiry function to conditiononally replace attached event */
  RUN_TEST(test_function_timer_static_expFn);
  RUN_TEST(test_function_timer_dynamic_expFn);

  /* Test a periodic timer w attached event and no expiry function ("tick") */
  RUN_TEST(test_function_timer_static_periodic_expire);
  RUN_TEST(test_function_timer_dynamic_periodic_expire);

  /* Test a periodic timer w attached message replaced every expiry (producer/consumer) */
  RUN_TEST(test_function_timer_dynamic_periodic_expire_expFn);

  UNITY_END();
}
