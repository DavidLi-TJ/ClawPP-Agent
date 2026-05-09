#ifndef CLAWPP_UI_VIEWS_LOG_VIEW_H
#define CLAWPP_UI_VIEWS_LOG_VIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QTimer>
#include <QFileSystemWatcher>
#include "infrastructure/logging/logger.h"

namespace clawpp {

class LogView : public QWidget {
    Q_OBJECT

public:
    explicit LogView(QWidget* parent = nullptr);
    
    void addLogEntry(LogLevel level, const QString& timestamp, const QString& message);
    void clearLogs();
    void refreshLogs();

private slots:
    void onFilterChanged(int index);
    void onSearchChanged(const QString& text);
    void onClearClicked();
    void onRefreshClicked();
    void onAutoRefreshToggled(bool checked);
    void onLogFileChanged(const QString& path);

private:
    void setupUi();
    void setupConnections();
    void applyFilter();
    void loadLogFile();
    QString getLogFilePath() const;
    LogLevel parseLogLevel(const QString& levelStr) const;

    QTableWidget* m_logTable;
    QComboBox* m_levelFilter;
    QLineEdit* m_searchEdit;
    QPushButton* m_clearBtn;
    QPushButton* m_refreshBtn;
    QPushButton* m_autoRefreshBtn;
    QFileSystemWatcher* m_watcher;
    QTimer* m_autoRefreshTimer;
    bool m_autoRefresh;
    
    struct LogEntry {
        LogLevel level;
        QString timestamp;
        QString message;
    };
    
    QList<LogEntry> m_allLogs;
};

}

#endif
