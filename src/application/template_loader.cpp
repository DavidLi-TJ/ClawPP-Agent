#include "template_loader.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include <QSet>

namespace clawpp {

TemplateLoader::TemplateLoader(const QString& workspace, const QString& kernelRoot, QObject* parent)
    : QObject(parent)
    , m_workspace(workspace)
    , m_kernelRoot(kernelRoot.isEmpty() ? workspace : kernelRoot)
    , m_cacheEnabled(true) {
    m_bootstrapFiles = {"IDENTITY.md", "AGENTS.md", "SOUL.md", "USER.md", "TOOLS.md", "HEARTBEAT.md"};
}

void TemplateLoader::setWorkspace(const QString& workspace) {
    m_workspace = workspace;
    m_cache.clear();
}

QString TemplateLoader::workspace() const {
    return m_workspace;
}

void TemplateLoader::setKernelRoot(const QString& kernelRoot) {
    m_kernelRoot = kernelRoot;
    m_cache.clear();
}

QString TemplateLoader::kernelRoot() const {
    return m_kernelRoot;
}

QString TemplateLoader::resolvePath(const QString& filename) const {
    if (QDir::isAbsolutePath(filename)) {
        return filename;
    }
    
    QString path = m_workspace + "/templates/" + filename;
    if (QFile::exists(path)) {
        return path;
    }
    
    path = m_workspace + "/" + filename;
    if (QFile::exists(path)) {
        return path;
    }
    
    if (filename.startsWith("memory/")) {
        path = m_workspace + "/" + filename;
        if (QFile::exists(path)) {
            return path;
        }
    }

    path = m_kernelRoot + "/templates/" + filename;
    if (QFile::exists(path)) {
        return path;
    }
    
    return QString();
}

QString TemplateLoader::readFile(const QString& path) const {
    if (path.isEmpty()) {
        return QString();
    }
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QString content = QString::fromUtf8(file.readAll());
    file.close();
    return content;
}

QString TemplateLoader::readFileHead(const QString& path, int maxLines, int maxBytes, bool* truncated) const {
    if (truncated) {
        *truncated = false;
    }
    if (path.isEmpty()) {
        return QString();
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    QStringList lines;
    int bytes = 0;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const int nextBytes = bytes + line.toUtf8().size() + 1;
        if (lines.size() >= maxLines || nextBytes > maxBytes) {
            if (truncated) {
                *truncated = true;
            }
            break;
        }
        lines.append(line);
        bytes = nextBytes;
    }

    if (!in.atEnd() && truncated) {
        *truncated = true;
    }
    file.close();
    return lines.join('\n');
}

QString TemplateLoader::loadTemplate(const QString& name) {
    if (m_cacheEnabled && m_cache.contains(name)) {
        return m_cache.value(name);
    }
    
    QString path = resolvePath(name);
    QString content = readFile(path);
    
    if (m_cacheEnabled && !content.isEmpty()) {
        m_cache.insert(name, content);
    }
    
    return content;
}

QString TemplateLoader::loadBootstrapFiles() {
    QStringList contents;
    
    for (const QString& file : m_bootstrapFiles) {
        QString content = loadTemplate(file);
        if (!content.isEmpty()) {
            contents.append(QString("=== %1 ===\n%2").arg(file, content));
        }
    }
    
    return contents.join("\n\n");
}

QString TemplateLoader::getIdentity() {
    return loadTemplate("SOUL.md");
}

QString TemplateLoader::loadMemory() {
    QString workspaceMemory = m_workspace + "/memory/MEMORY.md";
    bool truncated = false;
    QString content = readFileHead(workspaceMemory, 200, 25 * 1024, &truncated);
    if (!content.isEmpty()) {
        if (truncated) {
            content += QStringLiteral("\n\n[...memory truncated to first 200 lines / 25KB...]");
        }
        return content;
    }

    QString fallbackPath = resolvePath("memory/MEMORY.md");
    QString fallback = readFileHead(fallbackPath, 200, 25 * 1024, &truncated);
    if (!fallback.isEmpty() && truncated) {
        fallback += QStringLiteral("\n\n[...memory truncated to first 200 lines / 25KB...]");
    }
    return fallback;
}

QString TemplateLoader::loadHistory() {
    QString workspaceHistory = m_workspace + "/memory/HISTORY.md";
    QString content = readFile(workspaceHistory);
    if (content.isEmpty()) {
        return loadTemplate("memory/HISTORY.md");
    }

    QStringList lines = content.split('\n', Qt::SkipEmptyParts);
    const int maxLines = 40;
    if (lines.size() > maxLines) {
        lines = lines.mid(lines.size() - maxLines);
    }

    return lines.join("\n");
}

QString TemplateLoader::buildRuntimeContext(const QString& channel, const QString& chatId) {
    QStringList context;
    
    context.append("=== 当前运行时信息 ===");
    context.append(QString("时间: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")));
    
    if (!channel.isEmpty()) {
        context.append(QString("频道: %1").arg(channel));
    }
    
    if (!chatId.isEmpty()) {
        context.append(QString("会话ID: %1").arg(chatId));
    }
    
    context.append(QString("工作目录: %1").arg(m_workspace));
    
    QString memory = loadMemory();
    if (!memory.isEmpty()) {
        context.append("\n=== 长期记忆 ===");
        context.append(memory);
    }
    
    return context.join("\n");
}

QStringList TemplateLoader::discoverClaudeInstructionFiles() const {
    QStringList discovered;
    QSet<QString> seen;

    QDir current(QFileInfo(m_workspace).absoluteFilePath());
    if (!current.exists()) {
        return discovered;
    }

    while (true) {
        const QStringList candidates = {
            current.filePath(QStringLiteral("CLAUDE.md")),
            current.filePath(QStringLiteral(".claude/CLAUDE.md")),
            current.filePath(QStringLiteral("CLAUDE.local.md"))
        };

        for (const QString& candidate : candidates) {
            const QFileInfo fi(candidate);
            if (!fi.exists() || !fi.isFile()) {
                continue;
            }

            const QString normalized = QDir::cleanPath(fi.absoluteFilePath());
            if (seen.contains(normalized)) {
                continue;
            }
            seen.insert(normalized);
            discovered.append(normalized);
        }

        if (!current.cdUp()) {
            break;
        }
    }

    return discovered;
}

QString TemplateLoader::displayPath(const QString& absolutePath) const {
    const QString workspaceRoot = QDir(m_workspace).absolutePath();
    QString normalizedWorkspace = workspaceRoot;
    if (!normalizedWorkspace.endsWith('/')) {
        normalizedWorkspace += '/';
    }

    QString normalizedPath = QDir::cleanPath(absolutePath);
    if (normalizedPath.startsWith(normalizedWorkspace)) {
        return normalizedPath.mid(normalizedWorkspace.size());
    }
    if (normalizedPath == workspaceRoot) {
        return QStringLiteral(".");
    }
    return normalizedPath;
}

QString TemplateLoader::loadClaudeInstructions() {
    const QStringList paths = discoverClaudeInstructionFiles();
    if (paths.isEmpty()) {
        return QString();
    }

    QStringList sections;
    for (int i = paths.size() - 1; i >= 0; --i) {
        const QString path = paths.at(i);
        const QString content = readFile(path).trimmed();
        if (content.isEmpty()) {
            continue;
        }
        sections.append(QStringLiteral("## %1\n\n%2").arg(displayPath(path), content));
    }

    if (sections.isEmpty()) {
        return QString();
    }
    return sections.join(QStringLiteral("\n\n---\n\n"));
}

}
