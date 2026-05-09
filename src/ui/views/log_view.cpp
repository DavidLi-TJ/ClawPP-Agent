#include "log_view.h"
#include <QLabel>
#include <QHeaderView>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace clawpp {

LogView::LogView(QWidget* parent)
    : QWidget(parent)
    , m_logTable(nullptr)
    , m_levelFilter(nullptr)
    , m_searchEdit(nullptr)
    , m_clearBtn(nullptr)
    , m_refreshBtn(nullptr)
    , m_autoRefreshBtn(nullptr)
    , m_watcher(nullptr)
    , m_autoRefreshTimer(nullptr)
    , m_autoRefresh(false) {
    setupUi();
    setupConnections();
    loadLogFile();
    
    QString logPath = getLogFilePath();
    QFileInfo logFile(logPath);
    if (logFile.exists()) {
        m_watcher = new QFileSystemWatcher(this);
        m_watcher->addPath(logFile.absolutePath());
        connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &LogView::onLogFileChanged);
    }
}

void LogView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    setObjectName("logViewRoot");
    
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(16, 12, 16, 8);
    
    QLabel* levelLabel = new QLabel("级别：");
    levelLabel->setObjectName("logToolbarLabel");
    toolbarLayout->addWidget(levelLabel);
    
    m_levelFilter = new QComboBox();
    m_levelFilter->setObjectName("logLevelFilter");
    m_levelFilter->addItem("全部", -1);
    m_levelFilter->addItem("调试", static_cast<int>(LogLevel::Debug));
    m_levelFilter->addItem("信息", static_cast<int>(LogLevel::Info));
    m_levelFilter->addItem("警告", static_cast<int>(LogLevel::Warning));
    m_levelFilter->addItem("错误", static_cast<int>(LogLevel::Error));
    toolbarLayout->addWidget(m_levelFilter);
    
    toolbarLayout->addSpacing(16);
    
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索日志...");
    m_searchEdit->setObjectName("logSearchEdit");
    toolbarLayout->addWidget(m_searchEdit);
    
    toolbarLayout->addStretch();
    
    m_refreshBtn = new QPushButton("刷新");
    m_refreshBtn->setObjectName("logRefreshButton");
    toolbarLayout->addWidget(m_refreshBtn);
    
    m_autoRefreshBtn = new QPushButton("自动：关");
    m_autoRefreshBtn->setCheckable(true);
    m_autoRefreshBtn->setObjectName("logAutoRefreshButton");
    toolbarLayout->addWidget(m_autoRefreshBtn);
    
    m_clearBtn = new QPushButton("清空");
    m_clearBtn->setObjectName("logClearButton");
    toolbarLayout->addWidget(m_clearBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    m_logTable = new QTableWidget();
    m_logTable->setObjectName("logTable");
    m_logTable->setColumnCount(3);
    m_logTable->setHorizontalHeaderLabels({"时间", "级别", "消息"});
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_logTable->setAlternatingRowColors(true);
    m_logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logTable->verticalHeader()->setVisible(false);
    
    m_logTable->horizontalHeader()->setStretchLastSection(true);
    m_logTable->setColumnWidth(0, 180);
    m_logTable->setColumnWidth(1, 80);
    
    mainLayout->addWidget(m_logTable);
    
    m_autoRefreshTimer = new QTimer(this);
    m_autoRefreshTimer->setInterval(2000);
}

void LogView::setupConnections() {
    connect(m_levelFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LogView::onFilterChanged);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &LogView::onSearchChanged);
    connect(m_clearBtn, &QPushButton::clicked, this, &LogView::onClearClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &LogView::onRefreshClicked);
    connect(m_autoRefreshBtn, &QPushButton::toggled, this, &LogView::onAutoRefreshToggled);
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &LogView::onRefreshClicked);
}

QString LogView::getLogFilePath() const {
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return homeDir + "/.clawpp/logs/clawpp.log";
}

LogLevel LogView::parseLogLevel(const QString& levelStr) const {
    QString upper = levelStr.toUpper();
    if (upper == "DEBUG") return LogLevel::Debug;
    if (upper == "INFO") return LogLevel::Info;
    if (upper == "WARN" || upper == "WARNING") return LogLevel::Warning;
    if (upper == "ERROR") return LogLevel::Error;
    return LogLevel::Info;
}

void LogView::loadLogFile() {
    QString logPath = getLogFilePath();
    QFile file(logPath);
    
    if (!file.exists()) {
        return;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    
    m_allLogs.clear();
    m_logTable->setRowCount(0);
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        
        QRegularExpression re(R"(\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\]\s*\[(\w+)\]\s*(.*))");
        QRegularExpressionMatch match = re.match(line);
        
        if (match.hasMatch()) {
            QString timestamp = match.captured(1);
            QString levelStr = match.captured(2);
            QString message = match.captured(3);
            
            LogLevel level = parseLogLevel(levelStr);
            LogEntry entry{level, timestamp, message};
            m_allLogs.append(entry);
        }
    }
    
    file.close();
    applyFilter();
}

void LogView::refreshLogs() {
    loadLogFile();
}

void LogView::addLogEntry(LogLevel level, const QString& timestamp, const QString& message) {
    LogEntry entry{level, timestamp, message};
    m_allLogs.append(entry);
    
    int filterLevel = m_levelFilter->currentData().toInt();
    QString searchText = m_searchEdit->text().toLower();
    
    bool matchesLevel = (filterLevel == -1 || static_cast<int>(level) >= filterLevel);
    bool matchesSearch = searchText.isEmpty() || 
                         timestamp.toLower().contains(searchText) ||
                         message.toLower().contains(searchText);
    
    if (matchesLevel && matchesSearch) {
        int row = m_logTable->rowCount();
        m_logTable->insertRow(row);
        
        QTableWidgetItem* timeItem = new QTableWidgetItem(timestamp);
        timeItem->setForeground(QColor("#616161"));
        
        QTableWidgetItem* levelItem = new QTableWidgetItem();
        QString levelStr;
        QColor levelColor;
        switch (level) {
            case LogLevel::Debug:
                levelStr = "调试";
                levelColor = QColor("#9e9e9e");
                break;
            case LogLevel::Info:
                levelStr = "信息";
                levelColor = QColor("#1976d2");
                break;
            case LogLevel::Warning:
                levelStr = "警告";
                levelColor = QColor("#f57c00");
                break;
            case LogLevel::Error:
                levelStr = "错误";
                levelColor = QColor("#d32f2f");
                break;
        }
        levelItem->setText(levelStr);
        levelItem->setForeground(levelColor);
        levelItem->setFont(QFont("", -1, QFont::Bold));
        
        QTableWidgetItem* msgItem = new QTableWidgetItem(message);
        msgItem->setForeground(QColor("#424242"));
        
        m_logTable->setItem(row, 0, timeItem);
        m_logTable->setItem(row, 1, levelItem);
        m_logTable->setItem(row, 2, msgItem);
        
        m_logTable->scrollToBottom();
    }
}

void LogView::clearLogs() {
    m_allLogs.clear();
    m_logTable->setRowCount(0);
}

void LogView::onFilterChanged(int index) {
    Q_UNUSED(index);
    applyFilter();
}

void LogView::onSearchChanged(const QString& text) {
    Q_UNUSED(text);
    applyFilter();
}

void LogView::onClearClicked() {
    clearLogs();
}

void LogView::onRefreshClicked() {
    loadLogFile();
}

void LogView::onAutoRefreshToggled(bool checked) {
    m_autoRefresh = checked;
    m_autoRefreshBtn->setText(checked ? "自动：开" : "自动：关");
    
    if (checked) {
        m_autoRefreshTimer->start();
    } else {
        m_autoRefreshTimer->stop();
    }
}

void LogView::onLogFileChanged(const QString& path) {
    Q_UNUSED(path);
    if (m_autoRefresh) {
        loadLogFile();
    }
}

void LogView::applyFilter() {
    int filterLevel = m_levelFilter->currentData().toInt();
    QString searchText = m_searchEdit->text().toLower();
    
    m_logTable->setRowCount(0);
    
    for (const LogEntry& entry : m_allLogs) {
        bool matchesLevel = (filterLevel == -1 || static_cast<int>(entry.level) >= filterLevel);
        bool matchesSearch = searchText.isEmpty() ||
                             entry.timestamp.toLower().contains(searchText) ||
                             entry.message.toLower().contains(searchText);
        
        if (matchesLevel && matchesSearch) {
            int row = m_logTable->rowCount();
            m_logTable->insertRow(row);
            
            QTableWidgetItem* timeItem = new QTableWidgetItem(entry.timestamp);
            timeItem->setForeground(QColor("#616161"));
            
            QTableWidgetItem* levelItem = new QTableWidgetItem();
            QString levelStr;
            QColor levelColor;
            switch (entry.level) {
                case LogLevel::Debug:
                    levelStr = "调试";
                    levelColor = QColor("#9e9e9e");
                    break;
                case LogLevel::Info:
                    levelStr = "信息";
                    levelColor = QColor("#1976d2");
                    break;
                case LogLevel::Warning:
                    levelStr = "警告";
                    levelColor = QColor("#f57c00");
                    break;
                case LogLevel::Error:
                    levelStr = "错误";
                    levelColor = QColor("#d32f2f");
                    break;
            }
            levelItem->setText(levelStr);
            levelItem->setForeground(levelColor);
            levelItem->setFont(QFont("", -1, QFont::Bold));
            
            QTableWidgetItem* msgItem = new QTableWidgetItem(entry.message);
            msgItem->setForeground(QColor("#424242"));
            
            m_logTable->setItem(row, 0, timeItem);
            m_logTable->setItem(row, 1, levelItem);
            m_logTable->setItem(row, 2, msgItem);
        }
    }
    
    m_logTable->scrollToBottom();
}

}
