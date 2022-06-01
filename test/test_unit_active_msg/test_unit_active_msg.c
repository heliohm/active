#include <active.h>
#include <unity.h>

enum TestUserSignal
{
  TEST_SIG = USER_SIG
};

SIGNAL_DEFINE(sm, TEST_SIG);

const unsigned char payload[] = {0x1, 0x2, 0x3, 0x4};
const unsigned char payloadLen = sizeof(payload) / sizeof(payload[0]);

MESSAGE_DEFINE(mm, (uint16_t){0xBABA}, (void *)payload, payloadLen);

void test_macro_signal()
{
  TEST_ASSERT_EQUAL(sm.super.type, SIGNAL);
  TEST_ASSERT_EQUAL(sm.sig, TEST_SIG);
  TEST_ASSERT_FALSE(sm.super._dynamic);
  TEST_ASSERT_NULL(sm.super._sender);
}

void test_macro_message()
{
  TEST_ASSERT_EQUAL(MESSAGE, mm.super.type);
  TEST_ASSERT_EQUAL_UINT16(0xBABA, mm.header);
  TEST_ASSERT_EQUAL_PTR(payload, mm.payload);
  TEST_ASSERT_EQUAL(payloadLen, mm.payloadLen);
  TEST_ASSERT_FALSE(mm.super._dynamic);
  TEST_ASSERT_NULL(mm.super._sender);
}

void test_function_signal_init()
{
  Signal s;
  Active ao;
  Signal_init(&s, &ao, TEST_SIG);

  TEST_ASSERT_EQUAL(TEST_SIG, s.sig);
  TEST_ASSERT_EQUAL(SIGNAL, s.super.type);
  TEST_ASSERT_FALSE(s.super._dynamic);
  TEST_ASSERT_EQUAL(&ao, s.super._sender);
}

void test_function_message_init()
{
  Message m;
  Active ao;

  static const uint32_t msgPayload[] = {0xDEADBEEF, 0xBADDCAFE, 0xBAAAAAAD};
  static const size_t len = sizeof(msgPayload) / sizeof(msgPayload[0]);
  uint16_t header = 0xBABA;

  Message_init(&m, &ao, header, (void *)msgPayload, len);

  TEST_ASSERT_EQUAL_UINT16(header, m.header);
  TEST_ASSERT_EQUAL_PTR(&msgPayload, m.payload);
  TEST_ASSERT_EQUAL_UINT16(len, m.payloadLen);

  TEST_ASSERT_FALSE(m.super._dynamic);
  TEST_ASSERT_EQUAL(&ao, m.super._sender);
}

static Event *expFn(TimeEvt const *const te)
{
  return (Event *)NULL;
}

void test_function_timeevt_init()
{
  TimeEvt te;
  Active ao;
  Event e;

  TimeEvt_init(&te, &ao, &e, &ao, expFn);

  TEST_ASSERT_EQUAL(TIMEREVT, te.super.type);
  TEST_ASSERT_FALSE(te.super._dynamic);
  TEST_ASSERT_EQUAL(&ao, te.super._sender);
  TEST_ASSERT_EQUAL(&e, te.e);
  TEST_ASSERT_EQUAL(&ao, te.receiver);

  TEST_ASSERT_FALSE(te.timer.running);
  TEST_ASSERT_EQUAL(expFn, te.expFn);
  TEST_ASSERT_EQUAL(&te, te.timer.impl.user_data);
}

void test_macro_evt_upcast()
{

  Signal *s = NULL;

  TEST_ASSERT_EQUAL(EVT_UPCAST(s), (Event *)s);
}

void test_macro_evt_cast()
{
  Signal *s = NULL;
  Event *e = (Event *)s;

  TEST_ASSERT_EQUAL(s, EVT_CAST(e, Signal));
}

void main()
{
  k_msleep(2000);

  UNITY_BEGIN();

  RUN_TEST(test_macro_signal);
  RUN_TEST(test_macro_message);
  RUN_TEST(test_function_signal_init);
  RUN_TEST(test_function_message_init);
  RUN_TEST(test_function_timeevt_init);
  RUN_TEST(test_macro_evt_upcast);
  RUN_TEST(test_macro_evt_cast);

  UNITY_END();
}