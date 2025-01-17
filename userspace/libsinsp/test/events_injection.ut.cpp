#include <gtest/gtest.h>

#include "sinsp_with_test_input.h"
#include "test_utils.h"


static void encode_async_event(scap_evt* scapevt, uint64_t tid, const char* data)
{
	struct scap_plugin_params
	{};

	static uint32_t plug_id[2] = {sizeof (uint32_t), 0};

	size_t totlen = sizeof(scap_evt) + sizeof(plug_id)  + sizeof(uint32_t) + strlen(data) + 1;

	scapevt->tid = -1;
	scapevt->len = (uint32_t)totlen;
	scapevt->type = PPME_ASYNCEVENT_E;
	scapevt->nparams = 2;

	char* buff = (char *)scapevt + sizeof(struct ppm_evt_hdr);
	memcpy(buff, (char*)plug_id, sizeof(plug_id));
	buff += sizeof(plug_id);

	char* valptr = buff + sizeof(uint32_t);

	auto* data_len_ptr = (uint32_t*)buff;
	*data_len_ptr = (uint32_t)strlen(data) + 1;
	memcpy(valptr, data, *data_len_ptr);
}

class sinsp_evt_generator
{
private:
	struct scap_buff
	{
		uint8_t data[128];
	};

public:
	sinsp_evt_generator() = default;
	sinsp_evt_generator(sinsp_evt_generator&&) = default;
	~sinsp_evt_generator() = default;

	scap_evt* get(size_t idx)
	{
		return scap_ptrs[idx];
	}

	scap_evt* back()
	{
		return scap_ptrs.back();
	}

	std::unique_ptr<sinsp_evt> next(uint64_t ts = (uint64_t) -1)
	{
		scaps.emplace_back(new scap_buff());
		scap_ptrs.emplace_back((scap_evt*)scaps.back()->data);

		encode_async_event(scap_ptrs.back(), 1, "dummy_data");

		auto event = std::make_unique<sinsp_evt>();
		event->m_pevt = scap_ptrs.back();
		event->m_cpuid = 0;
		event->m_pevt->ts = ts;
		return event;
	};

private:
	std::vector<std::shared_ptr<scap_buff> > scaps;
	std::vector<scap_evt*> scap_ptrs; // gdb watch helper
};

TEST_F(sinsp_with_test_input, event_async_queue)
{
	open_inspector();
	m_inspector.m_lastevent_ts = 123;

	sinsp_evt_generator evt_gen;
	sinsp_evt* evt{};

	// inject event
	m_inspector.handle_async_event(evt_gen.next());

	// create test input event
	auto* scap_evt0 = add_event(increasing_ts(), 1, PPME_SYSCALL_OPEN_X, 6, (uint64_t)3, "/tmp/the_file",
					     PPM_O_RDWR, 0, 5, (uint64_t)123);

	// should pop injected event
	auto res = m_inspector.next(&evt);
	ASSERT_EQ(res, SCAP_SUCCESS);
	ASSERT_NE(evt, nullptr);
	ASSERT_EQ(evt->m_pevt, evt_gen.back());
	ASSERT_EQ(evt->m_pevt->ts, 123);
	ASSERT_TRUE(m_inspector.m_async_events_queue.empty());

	// multiple injected events
	m_inspector.m_lastevent_ts = scap_evt0->ts - 10;

	uint64_t injected_ts = scap_evt0->ts + 10;
	for (int i = 0; i < 10; ++i)
	{
		m_inspector.handle_async_event(evt_gen.next(injected_ts + i));
	}

	// create input[1] ivent
	auto* scap_evt1 = add_event(increasing_ts(), 1, PPME_SYSCALL_OPEN_X, 6, (uint64_t)3, "/tmp/the_file",
					     PPM_O_RDWR, 0, 5, (uint64_t)123);

	// pop scap 0 event
	res = m_inspector.next(&evt);
	ASSERT_EQ(res, SCAP_SUCCESS);
	ASSERT_EQ(evt->m_pevt, scap_evt0);
	auto last_ts = evt->m_pevt->ts;
	
	// pop injected
	for (int i= 0; i < 10; ++i)
	{
		res = m_inspector.next(&evt);
		ASSERT_EQ(res, SCAP_SUCCESS);
		ASSERT_EQ(evt->m_pevt, evt_gen.get(i+1));
		ASSERT_TRUE(last_ts <= evt->m_pevt->ts);
		last_ts = evt->m_pevt->ts;
	}
	ASSERT_TRUE(m_inspector.m_async_events_queue.empty());

	// pop scap 1
	res = m_inspector.next(&evt);
	ASSERT_EQ(res, SCAP_SUCCESS);
	ASSERT_EQ(evt->m_pevt, scap_evt1);
}

