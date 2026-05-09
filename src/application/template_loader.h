#ifndef CLAWPP_APPLICATION_TEMPLATE_LOADER_H
#define CLAWPP_APPLICATION_TEMPLATE_LOADER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

namespace clawpp {

class TemplateLoader : public QObject {
    Q_OBJECT

public:
    explicit TemplateLoader(const QString& workspace,
                            const QString& kernelRoot = QString(),
                            QObject* parent = nullptr);
    
    QString loadBootstrapFiles();
    QString getIdentity();
    QString buildRuntimeContext(const QString& channel = QString(), const QString& chatId = QString());
    QString loadTemplate(const QString& name);
    QString loadMemory();
    QString loadHistory();
    QString loadClaudeInstructions();
    
    void setWorkspace(const QString& workspace);
    QString workspace() const;
    void setKernelRoot(const QString& kernelRoot);
    QString kernelRoot() const;

private:
    QString readFile(const QString& path) const;
    QString readFileHead(const QString& path, int maxLines, int maxBytes, bool* truncated = nullptr) const;
    QString resolvePath(const QString& filename) const;
    QStringList discoverClaudeInstructionFiles() const;
    QString displayPath(const QString& absolutePath) const;

    QString m_workspace;
    QString m_kernelRoot;
    QStringList m_bootstrapFiles;
    QMap<QString, QString> m_cache;
    bool m_cacheEnabled;
};

}

#endif
