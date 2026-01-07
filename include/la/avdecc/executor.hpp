/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file executor.hpp
* @author Christophe Calmejane
* @brief Executor class to dispatch messages.
*/

#pragma once

#include "utils.hpp"
#include "internals/exports.hpp"

#include <memory>
#include <optional>
#include <string>
#include <functional>
#include <thread>
#include <future>
#include <chrono>
#include <stdexcept>
#include <exception>
#include <atomic>

namespace la
{
namespace avdecc
{
/**
 * @brief Executor class interface.
 * @details An Executor queues jobs and executes them in order at a later time.
*/
class Executor
{
public:
	using UniquePointer = std::unique_ptr<Executor, void (*)(Executor*)>;
	using Job = std::function<void()>;

	Executor() noexcept {}
	virtual ~Executor() noexcept {}

	/** Push a job to the executor. */
	virtual void pushJob(Job&& job) noexcept = 0;
	/** Flush all jobs in the executor, blocking until all jobs in the queue (at the moment of this call) are processed. */
	virtual void flush() noexcept = 0;
	/** Terminate the executor, flushing all jobs in the queue if flushJobs is true. */
	virtual void terminate(bool const flushJobs) noexcept = 0;
	/** Get the std::thread::id of the thread that is executing the jobs. */
	virtual std::thread::id getExecutorThread() const noexcept = 0;

	// Deleted compiler auto-generated methods
	Executor(Executor const&) = delete;
	Executor(Executor&&) = delete;
	Executor& operator=(Executor const&) = delete;
	Executor& operator=(Executor&&) = delete;
};

/** An Executor that proxies everything to provided functors. */
class ExecutorProxy : public Executor
{
public:
	using PushJobProxy = std::function<void(Job&&)>;
	using FlushProxy = std::function<void()>;
	using TerminateProxy = std::function<void(bool)>;
	using GetExecutorThreadProxy = std::function<std::thread::id()>;

	/**
	* @brief Factory method to create a new ExecutorProxy.
	* @details Creates a new ExecutorProxy as a unique pointer.
	* @return A new ExecutorProxy as a Executor::UniquePointer.
	*/
	static UniquePointer create(PushJobProxy const& pushJobProxy, FlushProxy const& flushProxy, TerminateProxy const& terminateProxy, GetExecutorThreadProxy const& getExecutorThreadProxy) noexcept
	{
		auto deleter = [](Executor* self)
		{
			static_cast<ExecutorProxy*>(self)->destroy();
		};
		return UniquePointer(createRawExecutorProxy(pushJobProxy, flushProxy, terminateProxy, getExecutorThreadProxy), deleter);
	}


	// Deleted compiler auto-generated methods
	ExecutorProxy(ExecutorProxy&&) = delete;
	ExecutorProxy(ExecutorProxy const&) = delete;
	ExecutorProxy& operator=(ExecutorProxy const&) = delete;
	ExecutorProxy& operator=(ExecutorProxy&&) = delete;

protected:
	/** Constructor */
	ExecutorProxy() noexcept = default;

	/** Destructor */
	virtual ~ExecutorProxy() noexcept = default;

private:
	/** Entry point */
	static LA_AVDECC_API ExecutorProxy* LA_AVDECC_CALL_CONVENTION createRawExecutorProxy(PushJobProxy const& pushJobProxy, FlushProxy const& flushProxy, TerminateProxy const& terminateProxy, GetExecutorThreadProxy const& getExecutorThreadProxy) noexcept;

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;
};

/** An Executor that executes jobs from a dispatch queue running in a separate thread. */
class ExecutorWithDispatchQueue : public Executor
{
public:
	/**
	* @brief Factory method to create a new ExecutorWithDispatchQueue.
	* @details Creates a new ExecutorWithDispatchQueue as a unique pointer.
	* @param[in] name An optional name for this Executor. If defined, will be used as the thread name.
	* @param[in] prio The priority of the thread.
	* @return A new ExecutorWithDispatchQueue as a Executor::UniquePointer.
	*/
	static UniquePointer create(std::optional<std::string> const& name = std::nullopt, utils::ThreadPriority const prio = utils::ThreadPriority::Normal) noexcept
	{
		auto deleter = [](Executor* self)
		{
			static_cast<ExecutorWithDispatchQueue*>(self)->destroy();
		};
		return UniquePointer(createRawExecutorWithDispatchQueue(name, prio), deleter);
	}

	// Deleted compiler auto-generated methods
	ExecutorWithDispatchQueue(ExecutorWithDispatchQueue&&) = delete;
	ExecutorWithDispatchQueue(ExecutorWithDispatchQueue const&) = delete;
	ExecutorWithDispatchQueue& operator=(ExecutorWithDispatchQueue const&) = delete;
	ExecutorWithDispatchQueue& operator=(ExecutorWithDispatchQueue&&) = delete;

protected:
	/** Constructor */
	ExecutorWithDispatchQueue() noexcept = default;

	/** Destructor */
	virtual ~ExecutorWithDispatchQueue() noexcept = default;

private:
	/** Entry point */
	static LA_AVDECC_API ExecutorWithDispatchQueue* LA_AVDECC_CALL_CONVENTION createRawExecutorWithDispatchQueue(std::optional<std::string> const& name, utils::ThreadPriority const prio) noexcept;

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;
};

/**
 * @brief Manager of Executors.
 * @details A singleton manager that holds Executors, referenced by a unique name.
 *          A single owner is allowed to register a new Executor and will get a ExecutorWrapper in return.
 *          That RAII wrapper will automatically remove and destroy the Executor from the manager when deleted.
*/
class ExecutorManager
{
public:
	/** Wrapper around an Executor for RAII removal from ExecutorManager. */
	class ExecutorWrapper
	{
	public:
		using UniquePointer = std::unique_ptr<ExecutorWrapper, void (*)(ExecutorWrapper*)>;

		/** Returns true if the wrapper contains a valid Executor */
		virtual explicit operator bool() const noexcept = 0;

		/** Push a new job to the wrapped Executor */
		virtual void pushJob(Executor::Job&& job) noexcept = 0;

		/** Flush the Executor */
		virtual void flush() noexcept = 0;

		// Deleted compiler auto-generated methods
		ExecutorWrapper(ExecutorWrapper&&) = delete;
		ExecutorWrapper(ExecutorWrapper const&) = delete;
		ExecutorWrapper& operator=(ExecutorWrapper const&) = delete;
		ExecutorWrapper& operator=(ExecutorWrapper&&) = delete;

		/** Destructor */
		virtual ~ExecutorWrapper() noexcept = default;

	protected:
		/** Constructor */
		ExecutorWrapper() noexcept = default;
	};

	/** Singleton instance */
	static LA_AVDECC_API ExecutorManager& LA_AVDECC_CALL_CONVENTION getInstance() noexcept;

	/** Checks if an Executor with the given name is already registered. */
	virtual bool isExecutorRegistered(std::string const& name) const noexcept = 0;

	/** Register a new Executor with the given name. Throws a std::runtime_error if an Executor with that name already exists. */
	virtual ExecutorWrapper::UniquePointer registerExecutor(std::string const& name, Executor::UniquePointer&& executor) = 0;

	/** Destroy an Executor with a given name. Returns true if the Executor was destroyed, false if it didn't exist. */
	virtual bool destroyExecutor(std::string const& name) noexcept = 0;

	/** Push the given job to the Executor with the given name. Silently ignored if the Executor does not exist. */
	virtual void pushJob(std::string const& name, Executor::Job&& job) noexcept = 0;

	/** Flush the Executor with the given name. Silently ignored if the Executor does not exist. */
	virtual void flush(std::string const& name) noexcept = 0;

	/** Get the std::thread::id of the Executor with the given name. Returns empty id if the Executor does not exist. */
	virtual std::thread::id getExecutorThread(std::string const& name) const noexcept = 0;

	/* See overload method below. */
	template<typename CallableType, typename Traits = utils::closure_traits<std::remove_reference_t<CallableType>>, typename Rep = std::int64_t, typename Period = std::milli>
	std::enable_if_t<Traits::arg_count == 0, typename Traits::result_type> waitJobResponse(std::string const& name, CallableType&& handler, std::chrono::duration<Rep, Period> timeout)
	{
		return waitJobResponse(name, std::forward<CallableType>(handler), std::optional<std::chrono::duration<Rep, Period>>(std::move(timeout)));
	}

	/**
	 * @brief Waits until the Executor with the given name has run the provided job.
	 * @details If the current thread is the Executor's thread, the job will skip the queue and be run immediately.
	 *          Otherwise, the job is queued and the function waits for its completion.
	 * @param[in] name The name of the Executor to run the job on.
	 * @param[in] handler The callable to execute. Must take no arguments.
	 * @param[in] timeout Optional timeout duration. If provided, the function will wait at most this long for the job to complete.
	 *                    If the timeout expires, a std::runtime_error is thrown.
	 *                    If not provided (default), the function waits indefinitely.
	 * @return The result of the handler if it returns a value, void otherwise.
	 * @throws std::invalid_argument If the executor with the given name does not exist, or if the handler throws an exception.
	 * @throws std::runtime_error If the timeout expires before the job completes.
	 * @note The timeout parameter can be any std::chrono::duration type (e.g., std::chrono::milliseconds, std::chrono::seconds).
	 *       It will be automatically converted to the appropriate optional type.
	 */
	template<typename CallableType, typename Traits = utils::closure_traits<std::remove_reference_t<CallableType>>, typename Rep = std::int64_t, typename Period = std::milli>
	std::enable_if_t<Traits::arg_count == 0, typename Traits::result_type> waitJobResponse(std::string const& name, CallableType&& handler, std::optional<std::chrono::duration<Rep, Period>> timeout = std::nullopt)
	{
		// If current thread is Executor thread, directly call handler
		if (std::this_thread::get_id() == getExecutorThread(name))
		{
			try
			{
				if constexpr (std::is_same_v<typename Traits::result_type, void>)
				{
					handler();
				}
				else
				{
					return handler();
				}
			}
			catch (...)
			{
				throw std::invalid_argument("Exception thrown in handler");
			}
		}
		else
		{
			if (!isExecutorRegistered(name))
			{
				throw std::invalid_argument("Executor not found");
			}
			auto responsePromise = std::promise<typename Traits::result_type>{};
			auto shouldIgnorePromise = std::make_shared<std::atomic<bool>>(false);
			pushJob(name,
				[&responsePromise, shouldIgnorePromise, h = std::forward<CallableType>(handler)]() mutable
				{
					try
					{
						// Always run the handler
						if constexpr (std::is_same_v<typename Traits::result_type, void>)
						{
							h();
							// Only set value if not timed out
							if (!shouldIgnorePromise->exchange(true))
							{
								responsePromise.set_value();
							}
						}
						else
						{
							auto const result = h();
							// Only set value if not timed out
							if (!shouldIgnorePromise->exchange(true))
							{
								responsePromise.set_value(result);
							}
						}
					}
					catch (...)
					{
						responsePromise.set_exception(std::make_exception_ptr(std::invalid_argument("Exception thrown in handler")));
					}
				});
			auto fut = responsePromise.get_future();
			if (timeout)
			{
				auto status = fut.wait_for(timeout.value());
				if (status == std::future_status::timeout)
				{
					// Job has timed out, mark the promise as to be ignored
					auto const alreadyProcessed = shouldIgnorePromise->exchange(true);
					// But in case the job just completed, we must not throw
					if (!alreadyProcessed)
					{
						throw std::runtime_error("Timeout waiting for job response");
					}
				}
			}
			// fut.get() waits for the job to complete and propagates std::invalid_argument if the handler threw an exception
			if constexpr (!std::is_same_v<typename Traits::result_type, void>)
			{
				return fut.get();
			}
			else
			{
				fut.get();
			}
		}
	}


	// Deleted compiler auto-generated methods
	ExecutorManager(ExecutorManager const&) = delete;
	ExecutorManager(ExecutorManager&&) = delete;
	ExecutorManager& operator=(ExecutorManager const&) = delete;
	ExecutorManager& operator=(ExecutorManager&&) = delete;

protected:
	ExecutorManager() noexcept = default;
	virtual ~ExecutorManager() noexcept = default;
};

} // namespace avdecc
} // namespace la
