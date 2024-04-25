/*
* Copyright (C) 2016-2024, L-Acoustics and its contributors

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
* @file executor.cpp
* @author Christophe Calmejane
* @brief Executor class to dispatch messages.
*/

#include "la/avdecc/executor.hpp"

#include <condition_variable>
#include <thread>
#include <mutex>
#include <deque>
#include <atomic>
#include <stdexcept>
#include <future>
#include <utility>
#include <unordered_map>

namespace la
{
namespace avdecc
{
class ExecutorProxyImpl final : public ExecutorProxy
{
public:
	ExecutorProxyImpl(PushJobProxy const& pushJobProxy, FlushProxy const& flushProxy, TerminateProxy const& terminateProxy, GetExecutorThreadProxy const& getExecutorThreadProxy) noexcept
		: ExecutorProxy{}
		, _pushJobProxy{ pushJobProxy }
		, _flushProxy{ flushProxy }
		, _terminateProxy{ terminateProxy }
		, _getExecutorThreadProxy{ getExecutorThreadProxy }
	{
	}
	virtual ~ExecutorProxyImpl() noexcept {}

	// Executor overrides
	virtual void pushJob(Job&& job) noexcept override
	{
		utils::invokeProtectedHandler(_pushJobProxy, std::move(job));
	}

	virtual void flush() noexcept override
	{
		utils::invokeProtectedHandler(_flushProxy);
	}

	virtual void terminate(bool const flushJobs = true) noexcept override
	{
		utils::invokeProtectedHandler(_terminateProxy, flushJobs);
	}

	virtual std::thread::id getExecutorThread() const noexcept override
	{
		return utils::invokeProtectedHandler(_getExecutorThreadProxy);
	}


	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override
	{
		delete this;
	}

	// Deleted compiler auto-generated methods
	ExecutorProxyImpl(ExecutorProxyImpl const&) = delete;
	ExecutorProxyImpl(ExecutorProxyImpl&&) = delete;
	ExecutorProxyImpl& operator=(ExecutorProxyImpl const&) = delete;
	ExecutorProxyImpl& operator=(ExecutorProxyImpl&&) = delete;

private:
	PushJobProxy _pushJobProxy{};
	FlushProxy _flushProxy{};
	TerminateProxy _terminateProxy{};
	GetExecutorThreadProxy _getExecutorThreadProxy{};
};

/** ExecutorProxy Entry point */
ExecutorProxy* LA_AVDECC_CALL_CONVENTION ExecutorProxy::createRawExecutorProxy(PushJobProxy const& pushJobProxy, FlushProxy const& flushProxy, TerminateProxy const& terminateProxy, GetExecutorThreadProxy const& getExecutorThreadProxy) noexcept
{
	return new ExecutorProxyImpl(pushJobProxy, flushProxy, terminateProxy, getExecutorThreadProxy);
}

class ExecutorWithDispatchQueueImpl final : public ExecutorWithDispatchQueue
{
public:
	ExecutorWithDispatchQueueImpl(std::optional<std::string> const& name = std::nullopt, utils::ThreadPriority const prio = utils::ThreadPriority::Normal) noexcept
		: ExecutorWithDispatchQueue{}
	{
		auto constructionComplete = std::promise<void>{};

		_executorThread = std::thread(
			[this, prio, name, &constructionComplete]
			{
				// Set the name of the thread, if specified
				if (name.has_value())
				{
					utils::setCurrentThreadName("Executor: " + *name);
				}
				// Set the priority of the thread
				utils::setCurrentThreadPriority(prio);

				// Signal that the thread is ready
				constructionComplete.set_value();

				// Run the thread, until termination is requested
				auto jobsToProcess = std::deque<Job>{};
				while (!_shouldTerminate)
				{
					// Wait for jobs to be available
					{
						// Wait for a dispatch operation to be available
						auto lock = std::unique_lock{ _executorLock };
						_executorCondVar.wait(lock,
							[this]
							{
								return _shouldTerminate || !_jobs.empty() || _flushingJobs;
							});

						// If termination is not requested and we didn't spuriously wake up
						if (!_shouldTerminate && !_jobs.empty())
						{
							// Move as many jobs as possible to the processing queue
							while (!_jobs.empty())
							{
								// We want to avoid a copy and there is no way to move a job out of a queue (as of C++17)
								// So we have to create a temporary (empty) job and swap it with the job in the queue
								// This is more effective than copying a full job
								auto job = Job{};
								std::swap(job, _jobs.front());
								_jobs.pop_front();
								jobsToProcess.push_back(std::move(job));
							}
						}
					}

					// If we are not terminating and we have jobs to process, process them outside the lock
					if (!_shouldTerminate && !jobsToProcess.empty())
					{
						// Process all jobs
						for (auto const& job : jobsToProcess)
						{
							utils::invokeProtectedHandler(job);
						}
						// Clear the processing queue
						jobsToProcess.clear();
					}

					// If we were asked to flush, notify that we are done
					// (It may happen that the _flushingJobs bool is set to true after we checked it in the wait condition, but reset it anyway as we processed some jobs and the flush() method will check for any other pending jobs)
					if (_flushingJobs)
					{
						auto const lg = std::lock_guard{ _executorLock };

						// Notify that we are done processing jobs
						_flushingJobs = false;
						_flushedPromise.set_value();
					}
				}
				_jobs.clear();
			});

		// Wait for thread running promise
		constructionComplete.get_future().wait();
	}

	virtual ~ExecutorWithDispatchQueueImpl() noexcept
	{
		// Properly terminate the executor thread
		terminate();
	}

	// Executor overrides
	virtual void pushJob(Job&& job) noexcept override
	{
		// Enter enqueue critical section, we don't want to enqueue new jobs if we are flushing
		auto const cs = std::lock_guard(_enqueueLock);

		// Check for termination
		if (_shouldTerminate)
		{
			return;
		}

		{
			auto const lg = std::lock_guard(_executorLock);

			// Enqueue the job
			_jobs.push_back(std::move(job));
		}

		// Notify the executor thread
		_executorCondVar.notify_one();
	}

	virtual void flush() noexcept override
	{
		// Enter enqueue critical section, preventing new jobs to be pushed
		auto const cs = std::lock_guard(_enqueueLock);

		// Wait until all jobs are processed. We must loop as one job might have been pushed while the executor is calling jobs (and will soon reset _flushingJobs)
		do
		{
			{
				auto const lg = std::lock_guard(_executorLock);

				// Set flag to indicate that we are flushing
				_flushingJobs = true;

				// Create a new promise to wait for the executor thread to finish processing the jobs
				_flushedPromise = std::promise<void>{};
			}

			// Notify the executor thread
			_executorCondVar.notify_one();

			// Wait for the executor thread to finish processing the jobs
			auto const fut = _flushedPromise.get_future();
			auto const ret = fut.wait_for(std::chrono::seconds(30));
			if (!AVDECC_ASSERT_WITH_RET(ret != std::future_status::timeout, "Executor timed out while flushing jobs"))
			{
				break;
			}
		} while (!_jobs.empty());
	}

	virtual void terminate(bool const flushJobs = true) noexcept override
	{
		// Enter enqueue critical section, preventing new jobs to be pushed
		auto const cs = std::lock_guard(_enqueueLock);

		// Flush jobs if requested
		if (flushJobs)
		{
			flush();
		}

		{
			auto const lg = std::lock_guard(_executorLock);

			// Discard (unflushed) jobs
			_jobs.clear();

			// Set termination flag
			_shouldTerminate = true;
		}

		// Notify the executor thread
		_executorCondVar.notify_one();

		// Wait for the thread to complete its pending tasks
		if (_executorThread.joinable())
		{
			_executorThread.join();
		}
	}

	virtual std::thread::id getExecutorThread() const noexcept override
	{
		return _executorThread.get_id();
	}

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override
	{
		delete this;
	}

	// Deleted compiler auto-generated methods
	ExecutorWithDispatchQueueImpl(ExecutorWithDispatchQueueImpl const&) = delete;
	ExecutorWithDispatchQueueImpl(ExecutorWithDispatchQueueImpl&&) = delete;
	ExecutorWithDispatchQueueImpl& operator=(ExecutorWithDispatchQueueImpl const&) = delete;
	ExecutorWithDispatchQueueImpl& operator=(ExecutorWithDispatchQueueImpl&&) = delete;

private:
	// Private members
	bool _shouldTerminate{ false }; // Flag to indicate that the executor thread should terminate
	bool _flushingJobs{ false }; // Flag to indicate we want to flush the jobs
	std::promise<void> _flushedPromise{}; // Promise to notify when the jobs have been flushed
	std::recursive_mutex _enqueueLock{}; // Lock to prevent new jobs to be pushed. We have to use a recursive lock to allow flush to be called from the destructor
	std::mutex _executorLock{}; // Lock to protect the executor queue
	std::deque<Job> _jobs{}; // Queue of jobs to be executed (could have used a std::queue but we want to be able to iterate and clear the queue)
	std::condition_variable _executorCondVar{}; // Condition variable to notify the executor thread
	std::condition_variable _jobProcessedCondVar{}; // Condition variable to notify the caller thread that a job has been processed
	std::thread _executorThread{}; // Thread running the executor
};

/** ExecutorWithDispatchQueue Entry point */
ExecutorWithDispatchQueue* LA_AVDECC_CALL_CONVENTION ExecutorWithDispatchQueue::createRawExecutorWithDispatchQueue(std::optional<std::string> const& name, utils::ThreadPriority const prio) noexcept
{
	return new ExecutorWithDispatchQueueImpl(name, prio);
}

class ExecutorManagerImpl final : public ExecutorManager
{
public:
	ExecutorManagerImpl() noexcept {}
	virtual ~ExecutorManagerImpl() noexcept {}

	// ExecutorManager overrides
	virtual bool isExecutorRegistered(std::string const& name) const noexcept override;
	virtual ExecutorWrapper::UniquePointer registerExecutor(std::string const& name, Executor::UniquePointer&& executor) override;
	virtual bool destroyExecutor(std::string const& name) noexcept override;
	virtual void pushJob(std::string const& name, Executor::Job&& job) noexcept override;
	virtual void flush(std::string const& name) noexcept override;
	virtual std::thread::id getExecutorThread(std::string const& name) const noexcept override;

	// Deleted compiler auto-generated methods
	ExecutorManagerImpl(ExecutorManagerImpl const&) = delete;
	ExecutorManagerImpl(ExecutorManagerImpl&&) = delete;
	ExecutorManagerImpl& operator=(ExecutorManagerImpl const&) = delete;
	ExecutorManagerImpl& operator=(ExecutorManagerImpl&&) = delete;

private:
	// Private members
	mutable std::mutex _lock{};
	std::unordered_map<std::string, Executor::UniquePointer> _executors{};
};

class ExecutorWrapperImpl final : public ExecutorManager::ExecutorWrapper
{
public:
	ExecutorWrapperImpl(Executor* const executor, std::string const& name, ExecutorManagerImpl* const manager) noexcept
		: _executor{ executor }
		, _name{ name }
		, _manager{ manager }
	{
	}
	virtual ~ExecutorWrapperImpl() noexcept
	{
		if (_manager != nullptr)
		{
			// Remove the executor from the ExecutorManager
			_manager->destroyExecutor(_name);
		}
	}

	// Deleted compiler auto-generated methods
	ExecutorWrapperImpl(ExecutorWrapperImpl const&) = delete;
	ExecutorWrapperImpl(ExecutorWrapperImpl&&) = delete;
	ExecutorWrapperImpl& operator=(ExecutorWrapperImpl const&) = delete;
	ExecutorWrapperImpl& operator=(ExecutorWrapperImpl&&) = delete;

private:
	friend class ExecutorManagerImpl;
	// ExecutorManager::ExecutorWrapper overrides
	/** Returns true if the wrapper contains a valid Executor */
	virtual explicit operator bool() const noexcept override
	{
		return _executor != nullptr;
	}

	/** Push a new job to the wrapped Executor */
	virtual void pushJob(Executor::Job&& job) noexcept override
	{
		if (_manager != nullptr)
		{
			// Use the ExecutorManager in order to guarantee correct synchronization in case of concurrent destruction.
			_manager->pushJob(_name, std::move(job));
		}
	}

	/** Flush the Executor */
	virtual void flush() noexcept override
	{
		if (_manager != nullptr)
		{
			// Use the ExecutorManager in order to guarantee correct synchronization in case of concurrent destruction.
			_manager->flush(_name);
		}
	}

	// Private members
	Executor* _executor{ nullptr };
	std::string _name{};
	ExecutorManagerImpl* _manager{ nullptr };
};

bool ExecutorManagerImpl::isExecutorRegistered(std::string const& name) const noexcept
{
	auto const lg = std::lock_guard(_lock);
	return _executors.find(name) != _executors.end();
}

ExecutorManager::ExecutorWrapper::UniquePointer ExecutorManagerImpl::registerExecutor(std::string const& name, Executor::UniquePointer&& executor)
{
	auto const lg = std::lock_guard(_lock);
	auto const result = _executors.try_emplace(name, std::move(executor));
	// If the insertion failed, throw an exception
	if (!result.second)
	{
		throw std::runtime_error{ "ExecutorManager: Executor with name '" + name + "' already exists" };
	}

	auto deleter = [](ExecutorWrapper* self)
	{
		delete static_cast<ExecutorWrapperImpl*>(self);
	};
	return ExecutorWrapper::UniquePointer(new ExecutorWrapperImpl{ result.first->second.get(), name, this }, deleter);
}

bool ExecutorManagerImpl::destroyExecutor(std::string const& name) noexcept
{
	auto const lg = std::lock_guard(_lock);
	if (auto const it = _executors.find(name); it != _executors.end())
	{
		_executors.erase(it);
		return true;
	}

	return false;
}

void ExecutorManagerImpl::pushJob(std::string const& name, Executor::Job&& job) noexcept
{
	auto const lg = std::lock_guard(_lock);
	if (auto const it = _executors.find(name); it != _executors.end())
	{
		it->second->pushJob(std::move(job));
	}
}

void ExecutorManagerImpl::flush(std::string const& name) noexcept
{
	auto const lg = std::lock_guard(_lock);
	if (auto const it = _executors.find(name); it != _executors.end())
	{
		it->second->flush();
	}
}

std::thread::id ExecutorManagerImpl::getExecutorThread(std::string const& name) const noexcept
{
	auto const lg = std::lock_guard(_lock);
	if (auto const it = _executors.find(name); it != _executors.end())
	{
		return it->second->getExecutorThread();
	}
	return {};
}

ExecutorManager& LA_AVDECC_CALL_CONVENTION ExecutorManager::getInstance() noexcept
{
	static auto s_instance = ExecutorManagerImpl{};
	return s_instance;
}

} // namespace avdecc
} // namespace la
