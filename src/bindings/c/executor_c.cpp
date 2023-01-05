/*
* Copyright (C) 2016-2023, L-Acoustics and its contributors

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
* @file executor_c.cpp
* @author Christophe Calmejane
* @brief C bindings for la::avdecc::Executor and la::avdecc::ExecutorManager.
*/

#include "la/avdecc/executor.hpp"
#include "la/avdecc/avdecc.h"
#include "utils.hpp"
//#include "config.h"
//#include <cstdlib>

/* ************************************************************************** */
/* Executor APIs                                                              */
/* ************************************************************************** */
static la::avdecc::bindings::HandleManager<la::avdecc::ExecutorManager::ExecutorWrapper::UniquePointer> s_ExecutorWrapperManager{};

LA_AVDECC_BINDINGS_C_API avdecc_executor_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_Executor_createQueueExecutor(avdecc_const_string_t executorName, LA_AVDECC_EXECUTOR_WRAPPER_HANDLE* const createdExecutorHandle)
{
	try
	{
		auto executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(executorName, la::avdecc::ExecutorWithDispatchQueue::create(executorName, la::avdecc::utils::ThreadPriority::Highest));
		*createdExecutorHandle = s_ExecutorWrapperManager.setObject(std::move(executorWrapper));
	}
	catch (std::runtime_error const&)
	{
		return static_cast<avdecc_executor_error_t>(avdecc_executor_error_already_exists);
	}

	return static_cast<avdecc_executor_error_t>(avdecc_executor_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_executor_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_Executor_destroy(LA_AVDECC_EXECUTOR_WRAPPER_HANDLE const handle)
{
	try
	{
		s_ExecutorWrapperManager.destroyObject(handle);
	}
	catch (std::runtime_error const&)
	{
		return static_cast<avdecc_executor_error_t>(avdecc_executor_error_not_found);
	}
	catch (std::invalid_argument const&)
	{
		return static_cast<avdecc_executor_error_t>(avdecc_executor_error_invalid_protocol_interface_handle);
	}

	return static_cast<avdecc_executor_error_t>(avdecc_protocol_interface_error_no_error);
}
