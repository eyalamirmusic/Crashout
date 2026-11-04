#include "ExceptionNames-Windows.h"
#include "CppExceptions-Windows.h"
#include "CrashReport.h"

#include <array>

namespace eacp::CrashReporting::Detail
{
namespace
{
constexpr auto heapCorruptionCode = DWORD {0xC0000374};
constexpr auto stackBufferOverrunCode = DWORD {0xC0000409};
constexpr auto floatMultipleTrapsCode = DWORD {0xC00002B5};
constexpr auto accessViolationParameterCount = DWORD {2};

struct AccessViolationKind
{
    ULONG_PTR operation;
    std::string_view description;
};

constexpr auto accessViolationKinds = std::to_array<AccessViolationKind>({
    {EXCEPTION_READ_FAULT, "read of address "},
    {EXCEPTION_WRITE_FAULT, "write to address "},
    {EXCEPTION_EXECUTE_FAULT, "execute at "},
});

struct ExceptionName
{
    DWORD code;
    std::string_view name;
};

constexpr auto exceptionNames = std::to_array<ExceptionName>({
    {EXCEPTION_ACCESS_VIOLATION, "EXCEPTION_ACCESS_VIOLATION"},
    {EXCEPTION_ARRAY_BOUNDS_EXCEEDED, "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"},
    {EXCEPTION_BREAKPOINT, "EXCEPTION_BREAKPOINT"},
    {EXCEPTION_DATATYPE_MISALIGNMENT, "EXCEPTION_DATATYPE_MISALIGNMENT"},
    {EXCEPTION_FLT_DENORMAL_OPERAND, "EXCEPTION_FLT_DENORMAL_OPERAND"},
    {EXCEPTION_FLT_DIVIDE_BY_ZERO, "EXCEPTION_FLT_DIVIDE_BY_ZERO"},
    {EXCEPTION_FLT_INEXACT_RESULT, "EXCEPTION_FLT_INEXACT_RESULT"},
    {EXCEPTION_FLT_INVALID_OPERATION, "EXCEPTION_FLT_INVALID_OPERATION"},
    {EXCEPTION_FLT_OVERFLOW, "EXCEPTION_FLT_OVERFLOW"},
    {EXCEPTION_FLT_STACK_CHECK, "EXCEPTION_FLT_STACK_CHECK"},
    {EXCEPTION_FLT_UNDERFLOW, "EXCEPTION_FLT_UNDERFLOW"},
    {floatMultipleTrapsCode, "STATUS_FLOAT_MULTIPLE_TRAPS"},
    {EXCEPTION_GUARD_PAGE, "STATUS_GUARD_PAGE_VIOLATION"},
    {EXCEPTION_ILLEGAL_INSTRUCTION, "EXCEPTION_ILLEGAL_INSTRUCTION"},
    {EXCEPTION_IN_PAGE_ERROR, "EXCEPTION_IN_PAGE_ERROR"},
    {EXCEPTION_INT_DIVIDE_BY_ZERO, "EXCEPTION_INT_DIVIDE_BY_ZERO"},
    {EXCEPTION_PRIV_INSTRUCTION, "EXCEPTION_PRIV_INSTRUCTION"},
    {EXCEPTION_STACK_OVERFLOW, "EXCEPTION_STACK_OVERFLOW"},
    {heapCorruptionCode, "STATUS_HEAP_CORRUPTION"},
    {stackBufferOverrunCode, "STATUS_STACK_BUFFER_OVERRUN"},
    {cppExceptionCode, "C++ exception"},
});

} // namespace

std::optional<std::string_view> findExceptionName(DWORD code)
{
    for (const auto& entry: exceptionNames)
    {
        if (entry.code == code)
            return entry.name;
    }

    return std::nullopt;
}

std::optional<std::string> describeAccessViolation(const EXCEPTION_RECORD& record)
{
    if (record.ExceptionCode != EXCEPTION_ACCESS_VIOLATION
        || record.NumberParameters < accessViolationParameterCount)
    {
        return std::nullopt;
    }

    const auto operation = record.ExceptionInformation[0];
    const auto address = record.ExceptionInformation[1];

    for (const auto& kind: accessViolationKinds)
    {
        if (kind.operation == operation)
            return std::string {kind.description} + formatHexAddress(address);
    }

    return "access of address " + formatHexAddress(address);
}
} // namespace eacp::CrashReporting::Detail
