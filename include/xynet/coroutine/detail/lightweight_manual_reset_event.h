//
// Created by xuanyi on 4/22/20.
//

/*
 * Copyright 2017 Lewis Baker
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef XYNET_COROUTINE_DETAIL_LIGHTWEIGHT_MANUAL_RESET_EVENT_H
#define XYNET_COROUTINE_DETAIL_LIGHTWEIGHT_MANUAL_RESET_EVENT_H

# include <atomic>
# include <cstdint>

namespace xynet
{
namespace detail
{
class lightweight_manual_reset_event
{
public:

  lightweight_manual_reset_event(bool initiallySet = false);

  ~lightweight_manual_reset_event();

  void set() noexcept;

  void reset() noexcept;

  void wait() noexcept;

private:
  std::atomic<int> m_value;

};
}
}


#include <system_error>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <linux/futex.h>
#include <cerrno>
#include <climits>
#include <cassert>

namespace
{
	namespace local
	{
		// No futex() function provided by libc.
		// Wrap the syscall ourselves here.
		int futex(
			int* UserAddress,
			int FutexOperation,
			int Value,
			const struct timespec* timeout,
			int* UserAddress2,
			int Value3)
		{
			return syscall(
				SYS_futex,
				UserAddress,
				FutexOperation,
				Value,
				timeout,
				UserAddress2,
				Value3);
		}
	}
}

xynet::detail::lightweight_manual_reset_event::lightweight_manual_reset_event(bool initiallySet)
	: m_value(initiallySet ? 1 : 0)
{}

xynet::detail::lightweight_manual_reset_event::~lightweight_manual_reset_event()
{
}

void xynet::detail::lightweight_manual_reset_event::set() noexcept
{
	m_value.store(1, std::memory_order_release);

	constexpr int numberOfWaitersToWakeUp = INT_MAX;

	[[maybe_unused]] int numberOfWaitersWokenUp = local::futex(
		reinterpret_cast<int*>(&m_value),
		FUTEX_WAKE_PRIVATE,
		numberOfWaitersToWakeUp,
		nullptr,
		nullptr,
		0);

	// There are no errors expected here unless this class (or the caller)
	// has done something wrong.
	assert(numberOfWaitersWokenUp != -1);
}

void xynet::detail::lightweight_manual_reset_event::reset() noexcept
{
	m_value.store(0, std::memory_order_relaxed);
}

void xynet::detail::lightweight_manual_reset_event::wait() noexcept
{
	// Wait in a loop as futex() can have spurious wake-ups.
	int oldValue = m_value.load(std::memory_order_acquire);
	while (oldValue == 0)
	{
		int result = local::futex(
			reinterpret_cast<int*>(&m_value),
			FUTEX_WAIT_PRIVATE,
			oldValue,
			nullptr,
			nullptr,
			0);
		if (result == -1)
		{
			if (errno == EAGAIN)
			{
				// The state was changed from zero before we could wait.
				// Must have been changed to 1.
				return;
			}

			// Other errors we'll treat as transient and just read the
			// value and go around the loop again.
		}

		oldValue = m_value.load(std::memory_order_acquire);
	}
}

#endif //XYNET_LIGHTWEIGHT_MANUAL_RESET_EVENT_H
