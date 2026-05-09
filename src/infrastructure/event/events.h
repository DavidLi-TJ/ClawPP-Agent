#ifndef CLAWPP_INFRASTRUCTURE_EVENT_EVENTS_H
#define CLAWPP_INFRASTRUCTURE_EVENT_EVENTS_H

#include <QString>
#include "common/types.h"

namespace clawpp {

struct MessageSentEvent {
    QString sessionId;
    QString content;
};

struct MessageReceivedEvent {
    QString sessionId;
    QString content;
    MessageRole role;
};

struct ToolCallEvent {
    QString sessionId;
    ToolCall toolCall;
};

struct ToolResultEvent {
    QString sessionId;
    ToolResult result;
};

struct SessionCreatedEvent {
    QString sessionId;
    QString name;
};

struct SessionDestroyedEvent {
    QString sessionId;
};

struct PermissionRequestEvent {
    QString sessionId;
    ToolCall toolCall;
    std::function<void(bool)> callback;
};

struct ErrorEvent {
    QString sessionId;
    QString message;
    QString details;
};

}

#endif
