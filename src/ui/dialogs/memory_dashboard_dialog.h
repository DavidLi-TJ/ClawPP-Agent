#ifndef CLAWPP_UI_MEMORY_DASHBOARD_DIALOG_H
#define CLAWPP_UI_MEMORY_DASHBOARD_DIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>

namespace clawpp {

class MemoryManager;

class MemoryDashboardDialog : public QDialog {
    Q_OBJECT

public:
    explicit MemoryDashboardDialog(MemoryManager* manager, QWidget* parent = nullptr);
    ~MemoryDashboardDialog() = default;

public slots:
    void refreshData();

private slots:
    void onClearMemoryClicked();
    void onForceCompressClicked();

private:
    void setupUi();

    MemoryManager* m_memoryManager;

    QTableWidget* m_historyTable;
    QProgressBar* m_tokenProgressBar;
    QLabel* m_compressionStatusLabel;
    QLabel* m_tokenCountLabel;
    
    QPushButton* m_refreshBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_compressBtn;
};

} // namespace clawpp

#endif // CLAWPP_UI_MEMORY_DASHBOARD_DIALOG_H
