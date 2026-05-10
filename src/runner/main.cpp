#include <QCoreApplication>
#include <iostream>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QProcess>
#include <QDir>
#include <QThread>
#include <QByteArray>
#include <QRegularExpression>
#include <QChar>
#include <QStringList>

static bool looksLikePowerShell(const QString& command) {
    static const QRegularExpression cmdPattern(
        QStringLiteral(R"((^|\s)(cd|dir|copy|move|del|type|mkdir|rmdir|python|pip|node|npm|pnpm|yarn|git|cmake|mingw32-make|make|cargo|go|java|javac)\b|&&|\|\|)"),
        QRegularExpression::CaseInsensitiveOption);
    if (cmdPattern.match(command).hasMatch()) {
        return false;
    }

    static const QRegularExpression psPattern(
        QStringLiteral(R"((^|\s)(Get|Set|New|Remove|Copy|Move|Rename|Start|Stop|Test|Select|Where|ForEach|Invoke|Write|Read)-[A-Za-z]|[$][A-Za-z_?]|[$]env:|[$]LASTEXITCODE|\$_|\bJoin-Path\b|\bOut-File\b|\bConvertTo-Json\b|\bConvertFrom-Json\b|\bStart-Process\b|\bSet-Location\b|\bPush-Location\b|\bPop-Location\b|\bGet-ChildItem\b|\bWhere-Object\b|\bSelect-Object\b|\bForEach-Object\b|@'|@\")"));
    return psPattern.match(command).hasMatch();
}

static QString adaptPowerShellCommand(const QString& command) {
    QString adapted = command;
    adapted.replace(QRegularExpression(QStringLiteral("(?<![&])&&(?![&])")),
                    QStringLiteral("; if ((-not $?) -or ($global:LASTEXITCODE -ne 0)) { if ($global:LASTEXITCODE -ne 0) { exit $global:LASTEXITCODE } else { exit 1 } }; "));
    return QStringLiteral("$ProgressPreference='SilentlyContinue'; [Console]::OutputEncoding=[System.Text.Encoding]::UTF8; $OutputEncoding=[System.Text.Encoding]::UTF8; ")
        + adapted;
}

QJsonObject handleShell(const QJsonObject& params) {
    QJsonObject result;
    QString command = params["command"].toString();
    QString cwd = params["cwd"].toString(QDir::currentPath());

    QProcess process;
    process.setWorkingDirectory(cwd);

#ifdef Q_OS_WIN
    if (looksLikePowerShell(command)) {
        const QString adapted = adaptPowerShellCommand(command);
        QByteArray encoded;
        encoded.reserve(adapted.size() * 2);
        for (QChar ch : adapted) {
            const ushort value = ch.unicode();
            encoded.append(static_cast<char>(value & 0xFF));
            encoded.append(static_cast<char>((value >> 8) & 0xFF));
        }
        const QString base64 = QString::fromLatin1(encoded.toBase64());
        process.start(QStringLiteral("powershell.exe"),
                      QStringList{
                          QStringLiteral("-NoProfile"),
                          QStringLiteral("-NonInteractive"),
                          QStringLiteral("-ExecutionPolicy"),
                          QStringLiteral("Bypass"),
                          QStringLiteral("-EncodedCommand"),
                          base64
                      });
    } else {
        process.start(QStringLiteral("cmd.exe"),
                      QStringList{
                          QStringLiteral("/d"),
                          QStringLiteral("/s"),
                          QStringLiteral("/c"),
                          command
                      });
    }
#else
    process.start(QStringLiteral("bash"),
                  QStringList{
                      QStringLiteral("-lc"),
                      command
                  });
#endif

    if (!process.waitForStarted(1000)) {
        result["error"] = "Process failed to start.";
        return result;
    }

    if (!process.waitForFinished(300000)) { // Max 5 mins fallback
        process.kill();
        process.waitForFinished(1000);
        result["error"] = "Process execution timed out.";
        result["stdout"] = QString::fromUtf8(process.readAllStandardOutput());
        result["stderr"] = QString::fromUtf8(process.readAllStandardError());
        return result;
    }

    const int exitCode = process.exitCode();
    QString stdoutText = QString::fromUtf8(process.readAllStandardOutput());
    QString stderrText = QString::fromUtf8(process.readAllStandardError());

#ifdef Q_OS_WIN
    if (exitCode == 0 && stderrText.trimmed().startsWith(QStringLiteral("#< CLIXML"))) {
        stderrText.clear();
    }
#endif

    result["stdout"] = stdoutText;
    result["stderr"] = stderrText;
    result["exit_code"] = exitCode;
    return result;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Read single line JSON-RPC from stdin
    std::string line;
    if (!std::getline(std::cin, line)) {
        return 1;
    }

    QByteArray input = QByteArray::fromStdString(line);
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(input, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QJsonObject err;
        err["error"] = "Invalid JSON input";
        std::cout << QJsonDocument(err).toJson(QJsonDocument::Compact).toStdString() << std::endl;
        return 1;
    }

    QJsonObject request = doc.object();
    QString action = request["action"].toString();
    QJsonObject params = request["params"].toObject();
    
    QJsonObject response;
    
    if (action == "shell") {
        response = handleShell(params);
    } else {
        response["error"] = QString("Unknown action: %1").arg(action);
    }
    
    // Write out the response as single JSON line to stdout
    std::cout << QJsonDocument(response).toJson(QJsonDocument::Compact).toStdString() << std::endl;
    
    return 0;
}
