#pragma once

#include <eacp/Core/Utils/WinInclude.h>

#include <optional>
#include <string>
#include <string_view>

namespace eacp::CrashReporting::Detail
{
// The NTSTATUS name of an exception code, which is also what a process that
// ended through one exits with.
std::optional<std::string_view> findExceptionName(DWORD code);

std::optional<std::string> describeAccessViolation(const EXCEPTION_RECORD& record);
} // namespace eacp::CrashReporting::Detail
