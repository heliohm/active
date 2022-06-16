#include <active.h>
#include <unity.h>

enum TestUserSignal
{
  TEST_SIG = ACT_USER_SIG
};

ACT_SIGNAL_DEFINE(sm, TEST_SIG);

void test_macro_signal()
{
  TEST_ASSERT_EQUAL(sm.super.type, ACT_SIGNAL);
  TEST_ASSERT_EQUAL(sm.sig, TEST_SIG);
  TEST_ASSERT_FALSE(sm.super._dynamic);
  TEST_ASSERT_NULL(sm.super._sender);
}

const unsigned char payload[] = {0x1, 0x2, 0x3, 0x4};
const unsigned char payloadLen = sizeof(payload) / sizeof(payload[0]);

ACT_MESSAGE_DEFINE(mm, (uint16_t){0xBABA}, (void *)payload, payloadLen);

void test_macro_message()
{
  TEST_ASSERT_EQUAL(ACT_MESSAGE, mm.super.type);
  TEST_ASSERT_EQUAL_UINT16(0xBABA, mm.header);
  TEST_ASSERT_EQUAL_PTR(payload, mm.payload);
  TEST_ASSERT_EQUAL(payloadLen, mm.payloadLen);
  TEST_ASSERT_FALSE(mm.super._dynamic);
  TEST_ASSERT_NULL(mm.super._sender);
}

void test_function_signal_init()
{
  ACT_Signal s;
  Active ao;
  ACT_Signal_init(&s, &ao, TEST_SIG);

  TEST_ASSERT_EQUAL(TEST_SIG, s.sig);
  TEST_ASSERT_EQUAL(ACT_SIGNAL, s.super.type);
  TEST_ASSERT_FALSE(s.super._dynamic);
  TEST_ASSERT_EQUAL(&ao, s.super._sender);
}

void test_function_message_init()
{
  ACT_Message m;
  Active ao;

  static const uint32_t msgPayload[] = {0xDEADBEEF, 0xBADDCAFE, 0xBAAAAAAD};
  static const size_t len = sizeof(msgPayload) / sizeof(msgPayload[0]);
  uint16_t header = 0xBABA;

  ACT_Message_init(&m, &ao, header, (void *)msgPayload, len);

  TEST_ASSERT_EQUAL_UINT16(header, m.header);
  TEST_ASSERT_EQUAL_PTR(&msgPayload, m.payload);
  TEST_ASSERT_EQUAL_UINT16(len, m.payloadLen);

  TEST_ASSERT_FALSE(m.super._dynamic);
  TEST_ASSERT_EQUAL(&ao, m.super._sender);
}

static ACT_Evt *expFn(ACT_TimEvt const *const te)
{
  return (ACT_Evt *)NULL;
}

void test_function_timeevt_init()
{
  ACT_TimEvt te;
  Active ao;
  ACT_Evt e;

  ACT_TimEvt_init(&te, &ao, &e, &ao, expFn);

  TEST_ASSERT_EQUAL(ACT_TIMEVT, te.super.type);
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

  ACT_Signal *s = NULL;

  TEST_ASSERT_EQUAL(EVT_UPCAST(s), (ACT_Evt *)s);
}

void test_macro_evt_cast()
{
  ACT_Signal *s = NULL;
  ACT_Evt *e = (ACT_Evt *)s;

  TEST_ASSERT_EQUAL(s, EVT_CAST(e, ACT_Signal));
}

void main()
{
  ACT_SLEEPMS(2000);

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