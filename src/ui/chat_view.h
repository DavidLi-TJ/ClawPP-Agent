#ifndef CLAWPP_UI_CHAT_VIEW_H
#define CLAWPP_UI_CHAT_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QScrollBar>
#include <QTimer>
#include <QKeyEvent>
#include <QMenu>
#include <QToolButton>
#include <QSet>
#include <QEvent>
#include "common/types.h"

namespace clawpp {

class MessageBubble;

/**
 * @class InputTextEdit
 * @brief 自定义输入文本框
 * 支持 Enter 发送、Ctrl+Enter 换行
 */
class InputTextEdit : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit InputTextEdit(QWidget* parent = nullptr);

signals:
    void sendRequested();  ///< 发送请求信号

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
};

/**
 * @class ChatView
 * @brief 聊天视图主界面
 * 
 * 负责显示消息、处理用户输入、与 Agent 交互
 * 包含完整的 ReAct 循环实现
 */
class ChatView : public QWidget {
    Q_OBJECT

public:
    explicit ChatView(QWidget* parent = nullptr);
    ~ChatView();

    void reloadProvider();  ///< 兼容接口：由服务层接管 provider 重新加载
    void loadMessages(const MessageList& messages);  ///< 加载消息列表
    void clearMessages();  ///< 清空消息
    MessageList messages() const;  ///< 获取消息列表
    
    void appendResponseChunk(const StreamChunk& chunk);  ///< 追加响应片段
    void finishStreaming();  ///< 完成流式输出
    void showError(const QString& error);  ///< 显示错误
    void appendSystemNotice(const QString& text);  ///< 追加系统提示
    void appendThinkingSegment(const QString& thought);  ///< 追加 Thinking 分段
    void appendActionSegment(const QString& action);     ///< 追加 Action 分段
    void appendObservationSegment(const QString& observation, bool success);  ///< 追加 Observation 分段
    void forceInputInteractive();  ///< 强制恢复输入框可交互（诊断/兜底）

signals:
    void messagesChanged(const MessageList& messages);  ///< 消息变化信号
    void messageSubmitted(const QString& text);         ///< 用户输入提交信号
    void stopRequested();                               ///< 停止生成请求信号
    void apiTestRequested();                            ///< API 测试请求信号
    void quickLoadFilesRequested();                     ///< 快捷：加载文件
    void quickClearContextRequested();                  ///< 快捷：清空上下文
    void quickCompressContextRequested();               ///< 快捷：压缩上下文
    void quickOpenMemoryRequested();                    ///< 快捷：查看记忆
    void quickExportSessionRequested();                 ///< 快捷：导出会话

private slots:
    void onSend();   ///< 发送按钮点击
    void onStop();   ///< 停止生成按钮点击

private:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void setupUi();  ///< 设置 UI 布局
    void setupQuickCommands();  ///< 设置快捷命令
    void updateStreamingUi(bool streaming);  ///< 统一更新流式状态相关 UI
    void sendMessage(const QString& text);  ///< 发送消息
    void onStreamChunk(const StreamChunk& chunk);  ///< 处理流式响应片段
    void onStreamComplete();  ///< 处理流式响应完成
    MessageBubble* addMessageBubble(const Message& message);  ///< 添加消息气泡
    void scrollToBottom();  ///< 滚动到底部
    void onStreamError(const QString& error);  ///< 处理流式错误

    QScrollArea* m_scrollArea;  ///< 滚动区域
    QWidget* m_messageContainer;  ///< 消息容器
    QVBoxLayout* m_messageLayout;  ///< 消息布局
    InputTextEdit* m_inputEdit;  ///< 输入框
    QLineEdit* m_fallbackInput;  ///< 兜底单行输入框（保障可输入）
    QPushButton* m_sendBtn;  ///< 发送按钮
    QPushButton* m_stopBtn;  ///< 停止按钮
    QToolButton* m_quickCmdBtn;  ///< 快捷命令按钮
    QMenu* m_quickCmdMenu;  ///< 快捷命令菜单
    MessageList m_messages;  ///< 消息列表
    MessageBubble* m_currentBubble;  ///< 当前消息气泡
    bool m_isStreaming;  ///< 是否正在流式响应
    bool m_followOutput; ///< 是否跟随到底部（用户上滑时关闭）
    int m_iteration;  ///< 当前迭代次数
    QString m_currentContent;  ///< 当前响应内容
    MessageBubble* m_currentThinkingBubble;  ///< 当前思考气泡
    QString m_currentThinkingContent;         ///< 当前思考内容
    QSet<QString> m_streamToolIdsNoticed;     ///< 已显示 action 的工具调用 ID
};

}

#endif
