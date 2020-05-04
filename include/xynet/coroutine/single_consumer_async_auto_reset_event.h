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

#ifndef CPPCORO_SINGLE_CONSUMER_ASYNC_AUTO_RESET_EVENT_HPP_INCLUDED
#define CPPCORO_SINGLE_CONSUMER_ASYNC_AUTO_RESET_EVENT_HPP_INCLUDED

#include <coroutine>
#include <atomic>
#include <cstdint>
#include <cassert>

namespace cppcoro
{
	class single_consumer_async_auto_reset_event
	{
	public:

		single_consumer_async_auto_reset_event(bool initiallySet = false) noexcept
			: m_state(initiallySet ? this : nullptr)
		{}

		void set() noexcept
		{
			void* oldValue = m_state.exchange(this, std::memory_order_release);
			if (oldValue != nullptr && oldValue != this)
			{
				// There was a waiting coroutine that we now need to resume.
				auto handle = *static_cast<std::coroutine_handle<>*>(oldValue);

				// We also need to transition the state back to 'not set' before
				// resuming the coroutine. This operation needs to be 'acquire'
				// so that it synchronises with other calls to .set() that execute
				// concurrently with this call and execute the above m_state.exchange(this)
				// operation with 'release' semantics.
				// This needs to be an exchange() instead of a store() so that it can have
				// 'acquire' semantics.
				(void)m_state.exchange(nullptr, std::memory_order_acquire);

				// Finally, resume the waiting coroutine.
				handle.resume();
			}
		}

		auto operator co_await() const noexcept
		{
			class awaiter
			{
			public:

				awaiter(const single_consumer_async_auto_reset_event& event) noexcept
					: m_event(event)
				{}

				bool await_ready() const noexcept { return false; }
				
				bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
				{
					m_awaitingCoroutine = awaitingCoroutine;

					void* oldValue = nullptr;
					if (!m_event.m_state.compare_exchange_strong(
						oldValue,
						&m_awaitingCoroutine,
						std::memory_order_release,
						std::memory_order_relaxed))
					{
						// This will only fail if the event was already 'set'
						// In which case we can just reset back to 'not set'
						// Need to use exchange() rather than store() here so we can make this
						// operation an 'acquire' operation so that we get visibility of all
						// writes prior to all preceding calls to .set().
						assert(oldValue == &m_event);
						(void)m_event.m_state.exchange(nullptr, std::memory_order_acquire);
						return false;
					}

					return true;
				}

				void await_resume() noexcept {}

			private:
				const single_consumer_async_auto_reset_event& m_event;
				std::coroutine_handle<> m_awaitingCoroutine;
			};

			return awaiter{ *this };
		}

	private:

		// nullptr - not set, no waiter
		// this    - set
		// other   - not set, pointer is address of a coroutine_handle<> to resume.
		mutable std::atomic<void*> m_state;

	};
}

#endif