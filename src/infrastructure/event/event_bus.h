#ifndef CLAWPP_INFRASTRUCTURE_EVENT_EVENT_BUS_H
#define CLAWPP_INFRASTRUCTURE_EVENT_EVENT_BUS_H

#include <QObject>
#include <QMap>
#include <functional>
#include <typeindex>
#include <any>
#include <exception>

namespace clawpp {

// Note: Template implementations are included in this header file because
// C++ standard requires template definitions to be visible at the point of
// instantiation. This is a standard practice for template-heavy code.

class EventBus : public QObject {
    Q_OBJECT

public:
    static EventBus& instance() {
        static EventBus instance;
        return instance;
    }
    
    template<typename Event>
    void subscribe(std::function<void(const Event&)> handler) {
        std::type_index idx(typeid(Event));
        m_handlers[idx].append([handler](const std::any& event) {
            try {
                handler(std::any_cast<Event>(event));
            } catch (const std::bad_any_cast&) {
                // 忽略类型不匹配的处理器，避免单个订阅者破坏事件总线。
            }
        });
    }
    
    template<typename Event>
    void publish(const Event& event) {
        std::type_index idx(typeid(Event));
        if (m_handlers.contains(idx)) {
            for (const auto& handler : m_handlers[idx]) {
                try {
                    handler(std::any(event));
                } catch (const std::bad_any_cast&) {
                    // 忽略处理器类型异常，继续分发给其他订阅者。
                }
            }
        }
    }
    
    void clear() {
        m_handlers.clear();
    }

private:
    EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    
    QMap<std::type_index, QList<std::function<void(const std::any&)>>> m_handlers;
};

}

#endif
