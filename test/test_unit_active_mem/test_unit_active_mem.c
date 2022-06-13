#include <active.h>
#include <unity.h>

enum TestUserSignal
{
  TEST_SIG = USER_SIG
};

void test_signal_new()
{
  Active ao;
  Signal *s = Signal_new(&ao, TEST_SIG);

  TEST_ASSERT_EQUAL(SIGNAL, s->super.type);
  TEST_ASSERT_EQUAL(&ao, s->super._sender);
  TEST_ASSERT_EQUAL(TEST_SIG, s->sig);
  TEST_ASSERT_TRUE(s->super._dynamic);
  TEST_ASSERT_EQUAL_UINT16(0, s->super._refcnt);

  ACT_mem_gc(EVT_UPCAST(s));
}

void test_message_new()
{
  Active ao;

  static const uint32_t msgPayload[] = {0xDEADBEEF, 0xBADDCAFE, 0xBAAAAAAD};
  static const size_t len = sizeof(msgPayload) / sizeof(msgPayload[0]);
  uint16_t header = 0xBABA;

  Message *m = Message_new(&ao, header, (void *)msgPayload, len);

  TEST_ASSERT_EQUAL_UINT16(header, m->header);
  TEST_ASSERT_EQUAL_PTR(&msgPayload, m->payload);
  TEST_ASSERT_EQUAL_UINT16(len, m->payloadLen);

  TEST_ASSERT_TRUE(m->super._dynamic);
  TEST_ASSERT_EQUAL(&ao, m->super._sender);

  ACT_mem_gc(EVT_UPCAST(m));
}

void test_timeevt_new()
{
  Active ao;
  Event e;

  TimeEvt *te = TimeEvt_new(&e, &ao, &ao, NULL);

  TEST_ASSERT_EQUAL(TIMEREVT, te->super.type);
  TEST_ASSERT_EQUAL(&ao, te->super._sender);
  TEST_ASSERT_TRUE(te->super._dynamic);
  TEST_ASSERT_EQUAL_UINT16(0, te->super._refcnt);

  TEST_ASSERT_EQUAL(&e, te->e);
  TEST_ASSERT_EQUAL(NULL, te->expFn);
  TEST_ASSERT_EQUAL(&ao, te->receiver);

  ACT_mem_gc(EVT_UPCAST(te));
}

void test_active_mem_gc()
{

  Active ao;

  for (uint16_t i = 0; i < ACT_MEM_NUM_SIGNALS + 10; i++)
  {
    uint32_t numUsed = ACT_mem_Signal_getUsed();
    Signal *s = Signal_new(&ao, TEST_SIG);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL(numUsed + 1, ACT_mem_Signal_getUsed());
    ACT_mem_gc(EVT_UPCAST(s));
    TEST_ASSERT_EQUAL(numUsed, ACT_mem_Signal_getUsed());
  }
}

void test_active_mem_ref_inc()
{
  Active ao;
  Signal *s = Signal_new(&ao, TEST_SIG);
  ACT_mem_refinc(EVT_UPCAST(s));
  TEST_ASSERT_EQUAL_UINT16(1, ACT_mem_getRefCount(EVT_UPCAST(s)));
}

void test_active_mem_ref_dec()
{
  Active ao;

  Signal *s = Signal_new(&ao, TEST_SIG);

  ACT_mem_refinc(EVT_UPCAST(s));
  ACT_mem_refdec(EVT_UPCAST(s));

  TEST_ASSERT_EQUAL_UINT16(0, ACT_mem_getRefCount(EVT_UPCAST(s)));
}

void main()
{
  k_msleep(2000);

  UNITY_BEGIN();

  RUN_TEST(test_active_mem_ref_inc);
  RUN_TEST(test_active_mem_ref_dec);

  RUN_TEST(test_signal_new);
  RUN_TEST(test_message_new);
  RUN_TEST(test_timeevt_new);
  RUN_TEST(test_active_mem_gc);

  UNITY_END();
}