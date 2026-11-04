#pragma once

#include <mach/mach.h>

#include <eacp/Core/Utils/Time.h>

#include <optional>

// The exchange that gives the handler the app's task port. The app spawns the
// handler with a send right to a port of its own as the handler's exception
// port for an exception nothing raises; the handler takes it and clears it,
// sends back a send right to its exception port, and the app answers with its
// task port. The bootstrap port is never borrowed: libxpc keeps the one it saw
// at process start, so a handler started on another port can never reach
// launchd services, and NSURLSession then fails every host lookup.
namespace eacp::CrashReporting::Detail
{
constexpr auto handshakeCarrierExceptionMask = exception_mask_t {EXC_MASK_RPC_ALERT};

struct HandshakePort
{
    mach_port_t receiveRight = MACH_PORT_NULL;
};

// The app's half, before the spawn: a port with a send right for the child.
std::optional<HandshakePort> createHandshakePort();
void destroyHandshakePort(HandshakePort& port);

// The app's half, after the spawn: waits for the handler's exception port,
// points this task's crash exceptions at it and answers with the task port.
// False when the handler never asked; the signal path is then all there is.
bool answerHandshakeWithTaskPort(const HandshakePort& port, Time::MS timeout);

// Puts back the exception ports the handshake replaced.
void restorePreviousExceptionPorts();

struct HandlerMachPorts
{
    mach_port_t exceptionPort = MACH_PORT_NULL;
    mach_port_t appTask = MACH_PORT_NULL;
};

// The handler's half, in two steps around its ready byte, so the wait for
// the app's answer never delays the app's start. Only on the channel that
// says the handshake port was passed.
struct PendingTaskPortRequest
{
    mach_port_t exceptionPort = MACH_PORT_NULL;
    mach_port_t replyPort = MACH_PORT_NULL;
};

std::optional<PendingTaskPortRequest> sendAppTaskPortRequest();

std::optional<HandlerMachPorts>
    receiveAppTaskPort(const PendingTaskPortRequest& request, Time::MS timeout);
} // namespace eacp::CrashReporting::Detail
