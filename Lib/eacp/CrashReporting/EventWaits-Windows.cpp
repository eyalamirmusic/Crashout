#include "CrashReportingPlatform.h"

#include <eacp/Core/Utils/WinInclude.h>

#include <winternl.h>

#include <vector>

namespace eacp::CrashReporting::Detail
{
namespace
{
constexpr auto threadStateWaiting = ULONG {5};
constexpr auto waitReasonWindowMessages = ULONG {13};
constexpr auto statusInfoLengthMismatch = NTSTATUS(0xC0000004L);
constexpr auto initialSnapshotBytes = ULONG {512 * 1024};

struct SystemThreadInformation
{
    LARGE_INTEGER kernelTime;
    LARGE_INTEGER userTime;
    LARGE_INTEGER createTime;
    ULONG waitTime;
    PVOID startAddress;
    CLIENT_ID clientId;
    LONG priority;
    LONG basePriority;
    ULONG contextSwitches;
    ULONG threadState;
    ULONG waitReason;
};

std::vector<std::byte> takeProcessSnapshot()
{
    auto snapshot = std::vector<std::byte>(initialSnapshotBytes);

    while (true)
    {
        auto needed = ULONG {0};
        auto status = ::NtQuerySystemInformation(SystemProcessInformation,
                                                 snapshot.data(),
                                                 static_cast<ULONG>(snapshot.size()),
                                                 &needed);

        if (status != statusInfoLengthMismatch)
            return NT_SUCCESS(status) ? snapshot : std::vector<std::byte> {};

        snapshot.resize(std::max<std::size_t>(needed, snapshot.size() * 2));
    }
}

bool isWaitingForWindowMessages(const SystemThreadInformation& thread)
{
    return thread.threadState == threadStateWaiting
           && thread.waitReason == waitReasonWindowMessages;
}
} // namespace

// GetMessage, WaitMessage and MsgWaitForMultipleObjects all park the thread in
// the kernel with this wait reason; a lock or Sleep never does.
bool isThreadWaitingForWindowMessages(std::uint64_t threadId)
{
    auto snapshot = takeProcessSnapshot();
    auto offset = std::size_t {0};

    while (offset < snapshot.size())
    {
        auto* process =
            reinterpret_cast<const SYSTEM_PROCESS_INFORMATION*>(&snapshot[offset]);
        auto* threads =
            reinterpret_cast<const SystemThreadInformation*>(process + 1);

        for (auto index = ULONG {0}; index < process->NumberOfThreads; ++index)
        {
            auto id = reinterpret_cast<std::uintptr_t>(
                threads[index].clientId.UniqueThread);

            if (id == threadId)
                return isWaitingForWindowMessages(threads[index]);
        }

        if (process->NextEntryOffset == 0)
            break;

        offset += process->NextEntryOffset;
    }

    return false;
}

void heartbeatMainThreadWhileWaitingForMessages(SharedHangInfo& hang,
                                                std::uint64_t now)
{
    auto& thread = hang.threads[mainThreadSlot];
    constexpr auto noGrace = std::uint64_t {0};

    if (findStallDuration(hang, thread, now, noGrace) == 0)
        return;

    if (isThreadWaitingForWindowMessages(thread.threadId))
        sendWatchedThreadHeartbeat(thread);
}
} // namespace eacp::CrashReporting::Detail
