/*
* L-Acoustics is the sole owner of any documents, designs, trademarks, logos,
* patents, know-hows, products, computer programs and software, models,
* technical sheets and any other such intellectual property rights
* whether registered or not mentioned or referred to in this document.

* The recipient of this document shall have no right, title, or interest
* in any of any documents, designs, trademarks, logos, patents, know-hows,
* products, computer programs and software, models, technical sheets
* and any other such intellectual property rights whether registered
* or not mentioned or referred to in this document.
* The recipient is not authorized to use, directly or indirectly, or grant
* any third party the right to use any of such intellectual property rights.

* Any copy or reproduction without prior written consent of L-Acoustics
* is forbidden and may lead to judicial prosecutions.
*/

#include <gtest/gtest.h>
#include <la/avdecc/executor.hpp>

#include <future>
#include <thread>
#include <chrono>

TEST(Executor, FlushJobs)
{
	auto constexpr ExecutorName = "ExecutorTest";
	static auto s_jobCompleted = false;

	// Create an executor with a dispatch queue
	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(ExecutorName, la::avdecc::utils::ThreadPriority::Highest));

	// Push a job that will sleep for 1 second so we are sure it will be being executed when we flush the jobs
	executorWrapper->pushJob(
		[]()
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			s_jobCompleted = true;
		});

	// Wait 500 msec to be sure the job is being executed
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// Flush the jobs
	executorWrapper->flush();

	// Check the job has been executed
	EXPECT_TRUE(s_jobCompleted);

	// Wait 750 msec to be sure the job completed before the executor is destroyed
	std::this_thread::sleep_for(std::chrono::milliseconds(750));
}

TEST(ExecutorManager, RegisterAndDestroyExecutor)
{
	auto constexpr ExecutorName = "TestExecutor";

	// Initially not registered
	EXPECT_FALSE(la::avdecc::ExecutorManager::getInstance().isExecutorRegistered(ExecutorName));

	// Register executor
	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	// Now registered
	EXPECT_TRUE(la::avdecc::ExecutorManager::getInstance().isExecutorRegistered(ExecutorName));

	// Destroy executor
	EXPECT_TRUE(la::avdecc::ExecutorManager::getInstance().destroyExecutor(ExecutorName));

	// No longer registered
	EXPECT_FALSE(la::avdecc::ExecutorManager::getInstance().isExecutorRegistered(ExecutorName));

	// Destroy again should return false
	EXPECT_FALSE(la::avdecc::ExecutorManager::getInstance().destroyExecutor(ExecutorName));
}

TEST(ExecutorManager, RegisterDuplicateExecutorThrows)
{
	auto constexpr ExecutorName = "DuplicateExecutor";

	auto executorWrapper1 = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	// Registering again should throw
	EXPECT_THROW(la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create()), std::runtime_error);
}

TEST(ExecutorManager, PushJobToNonExistentExecutor)
{
	auto constexpr ExecutorName = "NonExistent";

	// Should not crash, just silently ignore
	la::avdecc::ExecutorManager::getInstance().pushJob(ExecutorName,
		[]()
		{
		});
}

TEST(ExecutorManager, FlushNonExistentExecutor)
{
	auto constexpr ExecutorName = "NonExistent";

	// Should not crash, just silently ignore
	la::avdecc::ExecutorManager::getInstance().flush(ExecutorName);
}

TEST(ExecutorManager, GetExecutorThreadNonExistent)
{
	auto constexpr ExecutorName = "NonExistent";

	// Should return empty id
	EXPECT_EQ(std::thread::id{}, la::avdecc::ExecutorManager::getInstance().getExecutorThread(ExecutorName));
}

TEST(ExecutorManager, WaitJobResponseVoidNoTimeout)
{
	auto constexpr ExecutorName = "WaitTest";
	auto jobExecuted = false;

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
		[&jobExecuted]()
		{
			jobExecuted = true;
		});

	EXPECT_TRUE(jobExecuted);
}

TEST(ExecutorManager, WaitJobResponseVoidWithTimeout)
{
	auto constexpr ExecutorName = "WaitTestTimeout";
	auto jobExecuted = false;

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
		[&jobExecuted]()
		{
			jobExecuted = true;
		},
		std::chrono::milliseconds(100));
	EXPECT_TRUE(jobExecuted);
}

TEST(ExecutorManager, WaitJobResponseVoidTimeoutExpires)
{
	auto constexpr ExecutorName = "WaitTestTimeoutExpire";

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	// Push a job that sleeps longer than timeout
	EXPECT_THROW(la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
								 []()
								 {
									 std::this_thread::sleep_for(std::chrono::milliseconds(200));
								 },
								 std::chrono::milliseconds(50)),
		std::runtime_error);
}

TEST(ExecutorManager, WaitJobResponseIntReturn)
{
	auto constexpr ExecutorName = "WaitTestReturn";

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	auto const result = la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
		[]()
		{
			return 42u;
		});

	EXPECT_EQ(42u, result);
}

TEST(ExecutorManager, WaitJobResponseIntReturnWithTimeout)
{
	auto constexpr ExecutorName = "WaitTestReturnTimeout";

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	auto const result = la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
		[]()
		{
			return 42u;
		},
		std::chrono::milliseconds(100));

	EXPECT_EQ(42u, result);
}

TEST(ExecutorManager, WaitJobResponseHandlerThrows)
{
	auto constexpr ExecutorName = "WaitTestThrow";

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	EXPECT_THROW(la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
								 []()
								 {
									 throw std::runtime_error("Test exception");
								 }),
		std::invalid_argument);
}

TEST(ExecutorManager, WaitJobResponseHandlerThrowsWithReturn)
{
	auto constexpr ExecutorName = "WaitTestThrowReturn";

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	EXPECT_THROW(la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
								 []() -> std::uint32_t
								 {
									 throw std::runtime_error("Test exception");
									 return 0u;
								 }),
		std::invalid_argument);
}

TEST(ExecutorManager, WaitJobResponseFromExecutorThread)
{
	auto constexpr ExecutorName = "WaitTestSameThread";
	auto jobExecuted = false;

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	// Push a job that calls waitJobResponse from the executor thread
	executorWrapper->pushJob(
		[&jobExecuted]()
		{
			la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
				[&jobExecuted]()
				{
					jobExecuted = true;
				});
		});

	// Wait for the job to complete
	executorWrapper->flush();

	EXPECT_TRUE(jobExecuted);
}

TEST(ExecutorManager, WaitJobResponseWithSecondsTimeout)
{
	auto constexpr ExecutorName = "WaitTestSeconds";

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	auto jobExecuted = false;
	la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
		[&jobExecuted]()
		{
			jobExecuted = true;
		},
		std::chrono::seconds(1));

	EXPECT_TRUE(jobExecuted);
}

TEST(ExecutorManager, WaitJobResponseWithMillisecondsTimeout)
{
	auto constexpr ExecutorName = "WaitTestMilliseconds";

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	auto jobExecuted = false;
	la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
		[&jobExecuted]()
		{
			jobExecuted = true;
		},
		std::chrono::milliseconds(500));

	EXPECT_TRUE(jobExecuted);
}

TEST(ExecutorManager, WaitJobResponseTimeoutWithSeconds)
{
	auto constexpr ExecutorName = "WaitTestTimeoutSeconds";

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	EXPECT_THROW(la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
								 []()
								 {
									 std::this_thread::sleep_for(std::chrono::milliseconds(200));
								 },
								 std::chrono::seconds(0)),
		std::runtime_error);
}

TEST(ExecutorManager, WaitJobResponseTimeoutWithMilliseconds)
{
	auto constexpr ExecutorName = "WaitTestTimeoutMilliseconds";

	auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(ExecutorName, la::avdecc::ExecutorWithDispatchQueue::create());

	EXPECT_THROW(la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
								 []()
								 {
									 std::this_thread::sleep_for(std::chrono::milliseconds(200));
								 },
								 std::chrono::milliseconds(50)),
		std::runtime_error);
}

TEST(ExecutorManager, WaitJobResponseNonExistentExecutor)
{
	auto constexpr ExecutorName = "NonExistent";

	// Should throw std::invalid_argument immediately, without timing out
	EXPECT_THROW(la::avdecc::ExecutorManager::getInstance().waitJobResponse(ExecutorName,
								 []()
								 {
								 },
								 std::chrono::milliseconds(100)),
		std::invalid_argument);
}
