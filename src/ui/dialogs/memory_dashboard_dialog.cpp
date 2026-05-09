#include "memory_dashboard_dialog.h"
#include "../../memory/memory_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

namespace clawpp {

MemoryDashboardDialog::MemoryDashboardDialog(MemoryManager* manager, QWidget* parent)
    : QDialog(parent), m_memoryManager(manager)
{
    setObjectName("memoryDashboardDialog");
    setWindowTitle("记忆与上下文面板");
    setMinimumSize(600, 450);
    setupUi();
    refreshData();
}

void MemoryDashboardDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);
    
    // Status Layout
    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_tokenProgressBar = new QProgressBar(this);
    m_tokenProgressBar->setObjectName("memoryTokenProgressBar");
    m_tokenProgressBar->setRange(0, 8192); // Assuming 8k context visually
    m_tokenCountLabel = new QLabel("令牌：0 / 8192", this);
    m_tokenCountLabel->setObjectName("memoryTokenCountLabel");
    
    QLabel* statusLabel = new QLabel("上下文占用：", this);
    statusLabel->setObjectName("memoryStatusLabel");
    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(m_tokenProgressBar);
    statusLayout->addWidget(m_tokenCountLabel);
    
    // Summary line
    m_compressionStatusLabel = new QLabel("压缩状态：正常", this);
    m_compressionStatusLabel->setObjectName("memoryCompressionStatusLabel");
    
    // Table
    m_historyTable = new QTableWidget(0, 3, this);
    m_historyTable->setHorizontalHeaderLabels({"角色", "内容预览", "估算令牌"});
    m_historyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // Buttons Layout
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_refreshBtn = new QPushButton("刷新", this);
    m_compressBtn = new QPushButton("强制压缩上下文", this);
    m_clearBtn = new QPushButton("清空全部记忆", this);
    
    btnLayout->addWidget(m_refreshBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_compressBtn);
    btnLayout->addWidget(m_clearBtn);
    
    mainLayout->addLayout(statusLayout);
    mainLayout->addWidget(m_compressionStatusLabel);
    mainLayout->addWidget(m_historyTable);
    mainLayout->addLayout(btnLayout);
    
    connect(m_refreshBtn, &QPushButton::clicked, this, &MemoryDashboardDialog::refreshData);
    connect(m_compressBtn, &QPushButton::clicked, this, &MemoryDashboardDialog::onForceCompressClicked);
    connect(m_clearBtn, &QPushButton::clicked, this, &MemoryDashboardDialog::onClearMemoryClicked);
}

void MemoryDashboardDialog::refreshData() {
    if (!m_memoryManager) return;
    
    QList<Message> history = m_memoryManager->getAllMessages();
    m_historyTable->setRowCount(history.size());
    
    int totalTokens = 0;
    
    for (int i = 0; i < history.size(); ++i) {
        const Message& msg = history[i];
        
        QString roleStr;
        switch (msg.role) {
            case MessageRole::User: roleStr = "用户"; break;
            case MessageRole::Assistant: roleStr = "助手"; break;
            case MessageRole::System: roleStr = "系统"; break;
            case MessageRole::Tool: roleStr = "工具"; break;
            default: roleStr = "未知";
        }
        QTableWidgetItem* roleItem = new QTableWidgetItem(roleStr);
        
        QString preview = msg.content;
        if (preview.length() > 50) {
            preview = preview.left(47) + "...";
        }
        preview.replace("\n", " ");
        QTableWidgetItem* contentItem = new QTableWidgetItem(preview);
        
        // Very rough token estimation: rules of thumb 1.5 chars per token for chinese/mixed
        // Just for visualization
        int estTokens = msg.content.length() / 2; 
        totalTokens += estTokens;
        
        QTableWidgetItem* tokenItem = new QTableWidgetItem(QString::number(estTokens));
        
        m_historyTable->setItem(i, 0, roleItem);
        m_historyTable->setItem(i, 1, contentItem);
        m_historyTable->setItem(i, 2, tokenItem);
    }
    
    m_tokenProgressBar->setValue(qMin(totalTokens, 8192));
    m_tokenCountLabel->setText(QString("估算令牌：%1 / 8192（上限）").arg(totalTokens));
    
    if (totalTokens > 6000) {
        m_compressionStatusLabel->setText("压缩状态：紧急（接近上限）");
        m_compressionStatusLabel->setProperty("state", "warning");
    } else {
        m_compressionStatusLabel->setText("压缩状态：正常");
        m_compressionStatusLabel->setProperty("state", "normal");
    }

    m_compressionStatusLabel->style()->unpolish(m_compressionStatusLabel);
    m_compressionStatusLabel->style()->polish(m_compressionStatusLabel);
    m_compressionStatusLabel->update();
}

void MemoryDashboardDialog::onClearMemoryClicked() {
    if (QMessageBox::question(this, "清空记忆", "确定要清空当前会话记忆吗？", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_memoryManager->clear();
        refreshData();
    }
}

void MemoryDashboardDialog::onForceCompressClicked() {
    const bool compressed = m_memoryManager && m_memoryManager->forceCompress();
    if (compressed) {
        QMessageBox::information(this, "压缩上下文", "已执行手动压缩并生成中段摘要。");
    } else {
        QMessageBox::information(this, "压缩上下文", "当前上下文较短，暂无可压缩内容。");
    }
    refreshData();
}

} // namespace clawpp
