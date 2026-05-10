import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects

ApplicationWindow {
    id: root
    width: 1600
    height: 960
    visible: true
    title: "Claw++"
    color: "#F4F5F7"
    font.family: "Segoe UI Variable Text"

    property color bgTop: "#F2F2F7"
    property color bgBottom: "#EAEAEE"
    property color panelFill: Qt.rgba(240 / 255, 242 / 255, 248 / 255, settingPanelOpacity)
    property color panelBorder: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.30)
    property color textPrimary: "#111827"
    property color textSecondary: "#6B7280"
    property color accent: "#0066FF"
    property color accentSoft: Qt.rgba(0 / 255, 102 / 255, 255 / 255, 0.12)
    property string activePanel: "chat"
    property string settingApiKey: ""
    property string settingProviderType: "openai"
    property string settingBaseUrl: ""
    property string settingModel: ""
    property string settingTelegramToken: ""
    property string settingFeishuAppId: ""
    property string settingFeishuAppSecret: ""
    property string settingFeishuVerificationToken: ""
    property int settingFeishuPort: 8080
    property string settingDiscordWebhookUrl: ""
    property string settingDingTalkWebhookUrl: ""
    property string settingWechatWebhookUrl: ""
    property string settingQqWebhookUrl: ""
    property string settingWecomWebhookUrl: ""
    property string settingMessageChannel: "telegram"
    property int settingChatFontSize: 13
    property real settingChatLineHeight: 1.45
    property real settingPanelOpacity: 0.75
    property real settingBackgroundBlurRadius: 0.0
    property string settingBackgroundImagePath: ""
    property string settingsTestResult: ""
    property string settingsExternalTestResult: ""
    property string logsContent: ""
    property string memoryContent: ""
    property string historyContent: ""
    property string memorySearchQuery: ""
    property string memoryOverviewStatus: ""
    property real panelSwitchOpacity: 1.0
    property bool chatAutoScrollEnabled: true
    property bool chatScrollPending: false
    property string agentDocPath: "templates/SOUL.md"
    property string agentDocContent: ""
    property string agentDocStatus: ""
    property int compressionTokenCount: 0
    property int compressionThreshold: 0
    property int compressionContextLimit: 0
    property int compressionMessageCount: 0
    property bool compressionNeeds: false
    property string compressionStatusText: ""
    property int tokenLastPrompt: 0
    property int tokenLastCompletion: 0
    property int tokenLastTotal: 0
    property int tokenAccPrompt: 0
    property int tokenAccCompletion: 0
    property int tokenAccTotal: 0
    property string tokenStatsText: ""
    property int tokenChartDays: 30
    property int tokenSelectedYear: (new Date()).getFullYear()
    property int tokenResetYear: (new Date()).getFullYear()
    property int tokenResetMonth: (new Date()).getMonth() + 1
    property string tokenResetScope: "all"
    property string tokenResetSummary: ""
    property int tokenCurrentMonthTotal: 0
    property int tokenCurrentYearTotal: 0
    property int tokenAllTimeTotal: 0
    property int tokenDailyMax: 1
    property int tokenMonthlyMax: 1
    property int tokenYearlyMax: 1
    property bool tokenShowPrompt: true
    property bool tokenShowCompletion: true
    property bool tokenShowTotal: true
    property int tokenDailyViewStart: 0
    property int tokenDailyViewEnd: 0
    property int tokenHoveredDailyIndex: -1
    property int tokenHoveredMonthlyIndex: -1
    property int tokenHoveredYearlyIndex: -1
    property string tokenAnalyticsStatus: ""
    property string skillLoadErrorsText: ""
    property string renameSessionId: ""
    property var selectedSessionIds: []
    property int generatingDots: 0
    property string quickActionResult: ""

    function isChannelSelected(name) {
        return normalizeProviderType(settingMessageChannel) === normalizeProviderType(name)
    }

    function estimatedChatBubbleWidth(text, isUser, hasBreaks, availableWidth) {
        var value = text ? String(text) : ""
        var normalizedWidth = Math.max(300, availableWidth)
        var unit = Math.max(7.2, root.settingChatFontSize * 0.74)
        var padding = isUser ? 30 : 36
        var minWidth = isUser ? 90 : 130
        var maxWidth = isUser
            ? Math.min(normalizedWidth * 0.42, 380)
            : Math.min(normalizedWidth * 0.54, 520)
        var charCount = Math.min(84, value.length)
        var estimated = charCount * unit + padding
        if (hasBreaks) {
            estimated = Math.max(estimated, normalizedWidth * (isUser ? 0.22 : 0.3))
        }
        return Math.min(Math.max(minWidth, estimated), maxWidth)
    }

    ListModel {
        id: demoSessions
        ListElement { idValue: "demo-1"; name: "新聊天"; statusText: "active"; pinned: false; selected: true; updatedAt: "2026-03-29 10:00" }
        ListElement { idValue: "demo-2"; name: "设计讨论"; statusText: "active"; pinned: true; selected: false; updatedAt: "2026-03-29 09:42" }
    }

    ListModel {
        id: demoMessages
        ListElement { isUser: false; displayContent: "欢迎使用 Claw++，现在这套界面是统一的 Win11 半透明风格。" }
        ListElement { isUser: true; displayContent: "要左侧控制，右侧工作区，而且按钮要可用。" }
        ListElement { isUser: false; displayContent: "已处理：会话操作、设置、日志、memory、发送与停止都在主界面可直接操作。" }
    }

    ListModel {
        id: agentDocEntries
        ListElement { title: "系统 SOUL"; path: "templates/SOUL.md" }
        ListElement { title: "系统 AGENTS"; path: "templates/AGENTS.md" }
        ListElement { title: "系统 USER"; path: "templates/USER.md" }
        ListElement { title: "系统 TOOLS"; path: "templates/TOOLS.md" }
        ListElement { title: "系统 IDENTITY"; path: "templates/IDENTITY.md" }
        ListElement { title: "系统 HEARTBEAT"; path: "templates/HEARTBEAT.md" }
        ListElement { title: "Memory"; path: "memory/MEMORY.md" }
        ListElement { title: "History"; path: "memory/HISTORY.md" }
    }

    ListModel {
        id: skillsListModel
    }

    ListModel {
        id: memorySearchModel
    }

    ListModel {
        id: providerPresetModel
    }

    ListModel {
        id: providerModelOptions
    }

    ListModel {
        id: tokenDailyModel
    }

    ListModel {
        id: tokenMonthlyModel
    }

    ListModel {
        id: tokenYearlyModel
    }

    ListModel {
        id: tokenRecentEventsModel
    }

    ListModel {
        id: tokenYearOptionsModel
    }

    ListModel {
        id: tokenMonthOptionsModel
        ListElement { value: 1; label: "01" }
        ListElement { value: 2; label: "02" }
        ListElement { value: 3; label: "03" }
        ListElement { value: 4; label: "04" }
        ListElement { value: 5; label: "05" }
        ListElement { value: 6; label: "06" }
        ListElement { value: 7; label: "07" }
        ListElement { value: 8; label: "08" }
        ListElement { value: 9; label: "09" }
        ListElement { value: 10; label: "10" }
        ListElement { value: 11; label: "11" }
        ListElement { value: 12; label: "12" }
    }

    TextEdit {
        id: clipboardProxy
        visible: false
        width: 1
        height: 1
        readOnly: false
        textFormat: TextEdit.PlainText
    }

    GlassDialog {
        id: newSessionDialog
        title: "新建会话"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: 360

        onAccepted: {
            if (!backend) {
                return
            }

            var name = newSessionNameField.text.trim()
            if (name.length === 0) {
                name = "新会话"
            }
            backend.createSession(name)
            newSessionNameField.text = ""
        }

        onRejected: {
            newSessionNameField.text = ""
        }

        contentItem: ColumnLayout {
            spacing: 8

            Text {
                text: "输入会话名称"
                color: root.textSecondary
                font.pixelSize: 12
            }

            GlassTextField {
                id: newSessionNameField
                Layout.fillWidth: true
                placeholderText: "例如：需求评审"
            }
        }
    }

    GlassDialog {
        id: batchDeleteDialog
        title: "批量删除会话"
        modal: true
        width: 460
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (backend && backend.deleteSessionsByIds) {
                var deletedCount = backend.deleteSessionsByIds(root.selectedSessionIds)
                root.selectedSessionIds = []
                root.tokenAnalyticsStatus = "已删除 " + deletedCount + " 个会话"
            }
        }
        contentItem: ColumnLayout {
            spacing: 8
            Text {
                text: "将删除已勾选会话，共 " + root.selectedSessionIds.length + " 项。"
                color: root.textPrimary
                wrapMode: Text.WordWrap
            }
            Text {
                text: "此操作不可撤销。"
                color: "#B42318"
                font.pixelSize: 11
            }
        }
    }

    GlassDialog {
        id: renameSessionDialog
        title: "重命名会话"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: 360

        onAccepted: {
            if (backend && renameSessionId.length > 0) {
                backend.renameSessionById(renameSessionId, renameSessionNameField.text)
            }
            renameSessionId = ""
            renameSessionNameField.text = ""
        }

        onRejected: {
            renameSessionId = ""
            renameSessionNameField.text = ""
        }

        contentItem: ColumnLayout {
            spacing: 8

            Text {
                text: "输入新的会话名称"
                color: root.textSecondary
                font.pixelSize: 12
            }

            GlassTextField {
                id: renameSessionNameField
                Layout.fillWidth: true
                placeholderText: "例如：需求评审"
            }
        }
    }

    GlassDialog {
        id: tokenResetDialog
        title: "确认重置 Token 统计"
        modal: true
        width: 420
        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            root.applyTokenReset()
        }

        contentItem: ColumnLayout {
            spacing: 8

            Text {
                text: root.tokenResetSummary
                color: root.textPrimary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            Text {
                text: "该操作不可撤销。"
                color: "#B42318"
                font.pixelSize: 11
            }
        }
    }

    GlassDialog {
        id: probeResultDialog
        title: "API 测试结果"
        modal: true
        standardButtons: Dialog.Ok
        width: 420
        property string text: ""

        contentItem: ColumnLayout {
            spacing: 8
            Text {
                text: probeResultDialog.text
                color: root.textPrimary
                wrapMode: Text.WordWrap
                font.pixelSize: 12
            }
        }
    }

    GlassDialog {
        id: quickActionDialog
        title: "快捷操作"
        modal: true
        standardButtons: Dialog.Ok
        width: 620
        height: Math.min(root.height * 0.72, 560)
        property string text: ""

        contentItem: ScrollView {
            clip: true
            implicitWidth: quickActionDialog.width - 36
            implicitHeight: Math.min(360, quickActionText.implicitHeight + 16)
            Text {
                id: quickActionText
                width: quickActionDialog.width - 56
                text: quickActionDialog.text
                color: root.textPrimary
                wrapMode: Text.WrapAnywhere
                font.pixelSize: 12
            }
        }
    }

    GlassDialog {
        id: workflowDialog
        title: "启动工作流"
        modal: true
        width: 640
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string templateId: "research"
        property string modeId: "sequential"

        onAccepted: {
            if (!backend) {
                return
            }
            var task = workflowTaskField.text.trim()
            if (task.length === 0) {
                quickActionDialog.text = "请先填写工作流任务。"
                quickActionDialog.open()
                return
            }
            var result = backend.startAgentWorkflow(workflowDialog.templateId, workflowDialog.modeId, task)
            quickActionDialog.text = result
            quickActionDialog.open()
            workflowTaskField.text = ""
        }

        onRejected: {
            workflowTaskField.text = ""
        }

        contentItem: ColumnLayout {
            spacing: 8

            Text {
                text: "模板"
                color: root.textSecondary
                font.pixelSize: 12
            }

            GlassComboBox {
                id: workflowTemplateSelector
                Layout.fillWidth: true
                model: [
                    { label: "代理研究助手", value: "research" },
                    { label: "代理编码助手", value: "coding" }
                ]
                textRole: "label"
                onActivated: {
                    var row = model[currentIndex]
                    workflowDialog.templateId = row && row.value ? row.value : "research"
                }
            }

            Text {
                text: "执行模式"
                color: root.textSecondary
                font.pixelSize: 12
            }

            GlassComboBox {
                id: workflowModeSelector
                Layout.fillWidth: true
                model: [
                    { label: "单代理顺序执行", value: "sequential" },
                    { label: "子代理并行执行（简化）", value: "parallel" }
                ]
                textRole: "label"
                onActivated: {
                    var row = model[currentIndex]
                    workflowDialog.modeId = row && row.value ? row.value : "sequential"
                }
            }

            Text {
                text: "任务描述"
                color: root.textSecondary
                font.pixelSize: 12
            }

            TextArea {
                id: workflowTaskField
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                wrapMode: TextArea.Wrap
                placeholderText: "例如：调研某方案并输出可执行步骤；或在当前项目实现某功能并给出验证结果。"
            }
        }
    }

    FileDialog {
        id: backgroundImageDialog
        title: "选择背景图片"
        nameFilters: ["图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)", "All files (*)"]
        onAccepted: {
            root.settingBackgroundImagePath = selectedFile.toString().replace("file:///", "").replace(/\//g, "\\")
            settingsSaveTimer.restart()
        }
    }

    function normalizedFileUrl(pathText) {
        var p = pathText ? pathText.trim() : ""
        if (p.length === 0) {
            return ""
        }

        var normalized = p.replace(/\\/g, "/")
        if (normalized.indexOf("file://") === 0) {
            return normalized
        }
        return "file:///" + normalized
    }

    function sendCurrentInput() {
        if (!backend) {
            return
        }

        var text = inputField.text.trim()
        if (text.length === 0) {
            return
        }

        inputField.text = ""
        chatAutoScrollEnabled = true
        chatScrollPending = false
        chatScrollTimer.stop()
        backend.sendMessage(text)
        maybeScrollChatToEnd(true)
    }

    function maybeScrollChatToEnd(force) {
        if (!chatList) {
            return
        }
        if (!(force || chatAutoScrollEnabled)) {
            return
        }
        chatScrollPending = true
        if (!chatScrollTimer.running) {
            chatScrollTimer.start()
        }
    }

    Timer {
        id: chatScrollTimer
        interval: 20
        repeat: false
        onTriggered: {
            if (!root.chatScrollPending || !chatList) {
                return
            }
            root.chatScrollPending = false
            if (chatList.count <= 0 || chatList.moving || chatList.flicking || chatList.draggingVertically) {
                return
            }
            chatList.positionViewAtEnd()
        }
    }

    function providerPresetSeed() {
        return [
            {
                label: "OpenRouter",
                type: "openrouter",
                baseUrl: "https://openrouter.ai/api/v1",
                model: "openai/gpt-4.1-mini"
            },
            {
                label: "VolcEngine",
                type: "volcengine",
                baseUrl: "https://ark.cn-beijing.volces.com/api/v3",
                model: "doubao-1-5-lite-32k-250115"
            },
            {
                label: "BytePlus",
                type: "byteplus",
                baseUrl: "https://ark.byteintlapi.com/api/v3",
                model: "doubao-1-5-pro-32k-250115"
            },
            {
                label: "Anthropic(原生)",
                type: "anthropic",
                baseUrl: "https://api.anthropic.com",
                model: "claude-3-7-sonnet-20250219"
            },
            {
                label: "Azure OpenAI",
                type: "azure_openai",
                baseUrl: "https://{resource}.openai.azure.com",
                model: "gpt-4o-mini"
            },
            {
                label: "OpenAI",
                type: "openai",
                baseUrl: "https://api.openai.com/v1",
                model: "gpt-4o-mini"
            },
            {
                label: "DeepSeek",
                type: "deepseek",
                baseUrl: "https://api.deepseek.com/v1",
                model: "deepseek-chat"
            },
            {
                label: "Z.AI",
                type: "zai",
                baseUrl: "https://api.z.ai/v1",
                model: "glm-4.5"
            },
            {
                label: "Groq",
                type: "groq",
                baseUrl: "https://api.groq.com/openai/v1",
                model: "llama-3.3-70b-versatile"
            },
            {
                label: "MiniMax",
                type: "minimax",
                baseUrl: "https://api.minimax.chat/v1",
                model: "MiniMax-Text-01"
            },
            {
                label: "Gemini(原生)",
                type: "gemini",
                baseUrl: "https://generativelanguage.googleapis.com/v1beta",
                model: "gemini-2.5-flash"
            },
            {
                label: "AIHubMix",
                type: "aihubmix",
                baseUrl: "https://api.aihubmix.com/v1",
                model: "gpt-4o-mini"
            },
            {
                label: "SiliconFlow",
                type: "siliconflow",
                baseUrl: "https://api.siliconflow.cn/v1",
                model: "Qwen/Qwen2.5-72B-Instruct"
            },
            {
                label: "DashScope(Qwen)",
                type: "dashscope",
                baseUrl: "https://dashscope.aliyuncs.com/compatible-mode/v1",
                model: "qwen-plus"
            },
            {
                label: "Moonshot/Kimi",
                type: "moonshot",
                baseUrl: "https://api.moonshot.cn/v1",
                model: "moonshot-v1-8k"
            },
            {
                label: "Zhipu GLM",
                type: "zhipu",
                baseUrl: "https://open.bigmodel.cn/api/paas/v4",
                model: "glm-4-plus"
            },
            {
                label: "Ollama(local)",
                type: "ollama",
                baseUrl: "http://localhost:11434/v1",
                model: "qwen2.5:7b"
            },
            {
                label: "Mistral",
                type: "mistral",
                baseUrl: "https://api.mistral.ai/v1",
                model: "mistral-large-latest"
            },
            {
                label: "StepFun",
                type: "stepfun",
                baseUrl: "https://api.stepfun.com/v1",
                model: "step-2-16k"
            },
            {
                label: "OVMS(local)",
                type: "ovms",
                baseUrl: "http://localhost:8000/v3",
                model: "qwen2.5-7b-instruct"
            },
            {
                label: "vLLM(local)",
                type: "vllm",
                baseUrl: "http://localhost:8000/v1",
                model: "Qwen2.5-7B-Instruct"
            },
            {
                label: "OpenAI Codex",
                type: "openai_codex",
                baseUrl: "https://api.openai.com/v1",
                model: "codex-mini-latest"
            },
            {
                label: "GitHub Copilot",
                type: "github_copilot",
                baseUrl: "https://api.githubcopilot.com",
                model: "gpt-4o-mini"
            }
        ]
    }

    function normalizeProviderType(typeText) {
        var text = typeText ? String(typeText).trim().toLowerCase() : ""
        if (text === "openroutor") {
            text = "openrouter"
        }
        return text.length > 0 ? text : "openai"
    }

    function providerModelSeed(providerTypeText) {
        var providerType = normalizeProviderType(providerTypeText)
        if (providerType === "openrouter") return ["openai/gpt-4.1-mini", "anthropic/claude-3.5-sonnet", "google/gemini-2.5-flash"]
        if (providerType === "volcengine") return ["doubao-1-5-lite-32k-250115", "doubao-1-5-pro-32k-250115", "deepseek-r1-250120"]
        if (providerType === "byteplus") return ["doubao-1-5-pro-32k-250115", "doubao-1-5-lite-32k-250115", "deepseek-r1-250120"]
        if (providerType === "anthropic") return ["claude-3-7-sonnet-20250219", "claude-3-5-sonnet-20241022", "claude-3-5-haiku-20241022"]
        if (providerType === "azure_openai") return ["gpt-4o-mini", "gpt-4.1-mini", "o3-mini"]
        if (providerType === "openai") return ["gpt-4o-mini", "gpt-4.1-mini", "gpt-4.1", "o3-mini"]
        if (providerType === "deepseek") return ["deepseek-chat", "deepseek-reasoner"]
        if (providerType === "zai") return ["glm-4.5", "glm-4.5-air", "glm-4.5-flash"]
        if (providerType === "groq") return ["llama-3.3-70b-versatile", "deepseek-r1-distill-llama-70b", "qwen-qwq-32b"]
        if (providerType === "minimax") return ["MiniMax-Text-01", "MiniMax-M1", "abab6.5s-chat"]
        if (providerType === "gemini") return ["gemini-2.5-flash", "gemini-2.5-pro", "gemini-2.0-flash"]
        if (providerType === "aihubmix") return ["gpt-4o-mini", "claude-3.5-sonnet", "gemini-2.5-flash"]
        if (providerType === "siliconflow") return ["Qwen/Qwen2.5-72B-Instruct", "deepseek-ai/DeepSeek-V3", "moonshotai/Kimi-K2-Instruct"]
        if (providerType === "dashscope") return ["qwen-plus", "qwen-max", "qwq-plus"]
        if (providerType === "moonshot") return ["moonshot-v1-8k", "moonshot-v1-32k", "kimi-k2-0711-preview"]
        if (providerType === "zhipu") return ["glm-4-plus", "glm-4-flash", "glm-4-air"]
        if (providerType === "ollama") return ["qwen2.5:7b", "llama3.1:8b", "deepseek-r1:8b"]
        if (providerType === "mistral") return ["mistral-large-latest", "mistral-small-latest", "codestral-latest"]
        if (providerType === "stepfun") return ["step-2-16k", "step-1v-8k", "step-1-8k"]
        if (providerType === "ovms") return ["qwen2.5-7b-instruct", "llama-3.1-8b-instruct", "deepseek-r1-distill-qwen-7b"]
        if (providerType === "vllm") return ["Qwen2.5-7B-Instruct", "Llama-3.1-8B-Instruct", "DeepSeek-R1-Distill-Qwen-7B"]
        if (providerType === "openai_codex") return ["codex-mini-latest", "gpt-4.1", "gpt-4o-mini"]
        if (providerType === "github_copilot") return ["gpt-4o-mini", "claude-3.5-sonnet", "gemini-2.5-flash"]
        return ["gpt-4o-mini"]
    }

    function rebuildProviderPresetModel() {
        providerPresetModel.clear()
        var presets = providerPresetSeed()
        for (var i = 0; i < presets.length; ++i) {
            providerPresetModel.append(presets[i])
        }
    }

    function rebuildProviderModelOptions(providerTypeText) {
        providerModelOptions.clear()
        var models = providerModelSeed(providerTypeText)
        for (var i = 0; i < models.length; ++i) {
            providerModelOptions.append({ name: models[i] })
        }
    }

    function findProviderPresetIndex(providerTypeText, baseUrlText) {
        var typeTarget = normalizeProviderType(providerTypeText)
        var baseTarget = baseUrlText ? String(baseUrlText).trim() : ""

        for (var i = 0; i < providerPresetModel.count; ++i) {
            var row = providerPresetModel.get(i)
            if (row.type === typeTarget) {
                return i
            }
        }

        if (baseTarget.length > 0) {
            for (var j = 0; j < providerPresetModel.count; ++j) {
                if (providerPresetModel.get(j).baseUrl === baseTarget) {
                    return j
                }
            }
        }

        return -1
    }

    function findProviderModelIndex(modelName) {
        var target = modelName ? String(modelName).trim() : ""
        if (target.length === 0) {
            return -1
        }

        for (var i = 0; i < providerModelOptions.count; ++i) {
            if (providerModelOptions.get(i).name === target) {
                return i
            }
        }

        return -1
    }

    function currentProviderPresetLabel() {
        if (providerUrlSelector && providerUrlSelector.currentIndex >= 0 && providerUrlSelector.currentIndex < providerPresetModel.count) {
            var row = providerPresetModel.get(providerUrlSelector.currentIndex)
            return row && row.label ? row.label : ""
        }
        return settingProviderType + " · " + settingBaseUrl
    }

    function currentProviderModelLabel() {
        if (providerModelSelector && providerModelSelector.currentIndex >= 0 && providerModelSelector.currentIndex < providerModelOptions.count) {
            var row = providerModelOptions.get(providerModelSelector.currentIndex)
            return row && row.name ? row.name : ""
        }
        return settingModel
    }

    function syncProviderModelSelection() {
        if (!providerModelSelector) {
            return
        }

        var idx = findProviderModelIndex(settingModel)
        if (idx < 0 && settingModel && settingModel.trim().length > 0) {
            providerModelOptions.append({ name: settingModel.trim() })
            idx = providerModelOptions.count - 1
        } else if (idx < 0 && providerModelOptions.count > 0) {
            idx = 0
            settingModel = providerModelOptions.get(0).name
            if (modelField) {
                modelField.text = settingModel
            }
        }
        providerModelSelector.currentIndex = idx
    }

    function applyProviderPreset(index, useRecommendedModel) {
        if (index < 0 || index >= providerPresetModel.count) {
            return
        }

        var row = providerPresetModel.get(index)
        settingProviderType = row.type ? row.type : "openai"
        settingBaseUrl = row.baseUrl ? row.baseUrl : ""
        if (baseUrlField) {
            baseUrlField.text = settingBaseUrl
        }

        rebuildProviderModelOptions(settingProviderType)

        if (useRecommendedModel || !settingModel || settingModel.trim().length === 0) {
            var recommended = row.model && row.model.length > 0
                ? row.model
                : (providerModelOptions.count > 0 ? providerModelOptions.get(0).name : "")
            settingModel = recommended
            if (modelField) {
                modelField.text = settingModel
            }
        }

        syncProviderModelSelection()
    }

    function syncProviderPresetSelection(useRecommendedModel) {
        if (!providerUrlSelector) {
            return
        }

        var matchedIdx = findProviderPresetIndex(settingProviderType, settingBaseUrl)
        var idx = matchedIdx
        if (idx < 0) {
            idx = findProviderPresetIndex("openai", "")
        }
        if (idx < 0) {
            idx = 0
        }

        providerUrlSelector.currentIndex = idx
        if (useRecommendedModel) {
            applyProviderPreset(idx, true)
            return
        }

        if (matchedIdx < 0 && idx >= 0 && idx < providerPresetModel.count) {
            var fallback = providerPresetModel.get(idx)
            settingProviderType = fallback.type ? fallback.type : "openai"
            settingBaseUrl = fallback.baseUrl ? fallback.baseUrl : settingBaseUrl
        }
        rebuildProviderModelOptions(settingProviderType)
        syncProviderModelSelection()
        if (baseUrlField) {
            baseUrlField.text = settingBaseUrl
        }
    }

    function loadSettingsContent() {
        if (backend && backend.getSettings) {
            var settings = backend.getSettings()
            settingApiKey = settings.apiKey ? settings.apiKey : ""
            settingProviderType = settings.providerType ? normalizeProviderType(settings.providerType) : "openai"
            settingBaseUrl = settings.baseUrl ? settings.baseUrl : ""
            settingModel = settings.model ? settings.model : ""
            rebuildProviderPresetModel()
            settingTelegramToken = settings.telegramToken ? settings.telegramToken : ""
            settingFeishuAppId = settings.feishuAppId ? settings.feishuAppId : ""
            settingFeishuAppSecret = settings.feishuAppSecret ? settings.feishuAppSecret : ""
            settingFeishuVerificationToken = settings.feishuVerificationToken ? settings.feishuVerificationToken : ""
            settingFeishuPort = settings.feishuPort ? settings.feishuPort : 8080
            settingDiscordWebhookUrl = settings.discordWebhookUrl ? settings.discordWebhookUrl : ""
            settingDingTalkWebhookUrl = settings.dingtalkWebhookUrl ? settings.dingtalkWebhookUrl : ""
            settingWechatWebhookUrl = settings.wechatWebhookUrl ? settings.wechatWebhookUrl : ""
            settingQqWebhookUrl = settings.qqWebhookUrl ? settings.qqWebhookUrl : ""
            settingWecomWebhookUrl = settings.wecomWebhookUrl ? settings.wecomWebhookUrl : ""
            settingMessageChannel = settings.messageChannel ? settings.messageChannel : "telegram"
            settingChatFontSize = settings.chatFontSize ? Math.max(11, Math.min(20, Number(settings.chatFontSize))) : 13
            settingChatLineHeight = settings.chatLineHeight ? Math.max(1.15, Math.min(2.0, Number(settings.chatLineHeight))) : 1.45
            settingPanelOpacity = settings.panelOpacity ? Math.max(0.35, Math.min(0.95, Number(settings.panelOpacity))) : 0.75
            settingBackgroundBlurRadius = settings.backgroundBlurRadius ? Math.max(0, Math.min(36, Number(settings.backgroundBlurRadius))) : 0
            var savedBgPath = settings.backgroundImagePath ? settings.backgroundImagePath : ""
            if (savedBgPath.length > 0) {
                settingBackgroundImagePath = savedBgPath
                cachedBgPath = savedBgPath
            }

            if (apiKeyField) apiKeyField.text = settingApiKey
            if (baseUrlField) baseUrlField.text = settingBaseUrl
            if (modelField) modelField.text = settingModel
            if (telegramField) telegramField.text = settingTelegramToken
            if (feishuAppIdField) feishuAppIdField.text = settingFeishuAppId
            if (feishuSecretField) feishuSecretField.text = settingFeishuAppSecret
            if (feishuVerifyField) feishuVerifyField.text = settingFeishuVerificationToken
            if (feishuPortField) feishuPortField.value = settingFeishuPort
            if (discordWebhookField) discordWebhookField.text = settingDiscordWebhookUrl
            if (dingtalkWebhookField) dingtalkWebhookField.text = settingDingTalkWebhookUrl
            if (wechatWebhookField) wechatWebhookField.text = settingWechatWebhookUrl
            if (qqWebhookField) qqWebhookField.text = settingQqWebhookUrl
            if (wecomWebhookField) wecomWebhookField.text = settingWecomWebhookUrl
            if (chatFontSizeSlider) chatFontSizeSlider.value = settingChatFontSize
            if (chatLineHeightSlider) chatLineHeightSlider.value = settingChatLineHeight
            syncProviderPresetSelection(false)
        }
        loadAgentDocContent(agentDocPath)
        settingsTestResult = ""
        settingsExternalTestResult = ""
        if (messageChannelSelector) {
            var channelIndex = messageChannelSelector.model.indexOf(settingMessageChannel)
            messageChannelSelector.currentIndex = channelIndex >= 0 ? channelIndex : 0
        }
    }

    function loadAgentDocContent(pathText) {
        if (!(backend && backend.readWorkspaceFile)) {
            agentDocStatus = "后端未就绪，无法加载文档。"
            agentDocContent = ""
            return
        }

        var targetPath = pathText && pathText.length > 0 ? pathText : agentDocPath
        if (targetPath.length === 0) {
            return
        }

        agentDocPath = targetPath
        var text = backend.readWorkspaceFile(targetPath)
        agentDocContent = text
        if (text.length > 0) {
            agentDocStatus = "已加载: " + targetPath
        } else {
            agentDocStatus = "文件为空或不存在: " + targetPath
        }
    }

    function saveAgentDocContent() {
        if (!(backend && backend.writeWorkspaceFile)) {
            agentDocStatus = "后端未就绪，无法保存文档。"
            return
        }

        if (!agentDocPath || agentDocPath.length === 0) {
            agentDocStatus = "未选择文档路径。"
            return
        }

        var ok = backend.writeWorkspaceFile(agentDocPath, agentDocContent)
        agentDocStatus = ok ? ("已保存: " + agentDocPath) : ("保存失败: " + agentDocPath)
    }

    function saveSettingsContent() {
        if (backend && backend.applySettings) {
            backend.applySettings({
                apiKey: settingApiKey,
                providerType: normalizeProviderType(settingProviderType),
                baseUrl: settingBaseUrl,
                model: settingModel,
                telegramToken: settingTelegramToken,
                feishuAppId: settingFeishuAppId,
                feishuAppSecret: settingFeishuAppSecret,
                feishuVerificationToken: settingFeishuVerificationToken,
                feishuPort: settingFeishuPort,
                discordWebhookUrl: settingDiscordWebhookUrl,
                dingtalkWebhookUrl: settingDingTalkWebhookUrl,
                wechatWebhookUrl: settingWechatWebhookUrl,
                qqWebhookUrl: settingQqWebhookUrl,
                wecomWebhookUrl: settingWecomWebhookUrl,
                messageChannel: settingMessageChannel,
                chatFontSize: settingChatFontSize,
                chatLineHeight: settingChatLineHeight,
                panelOpacity: settingPanelOpacity,
                backgroundBlurRadius: settingBackgroundBlurRadius,
                backgroundImagePath: settingBackgroundImagePath
            })
            settingsTestResult = "设置已保存并应用。"
        }
    }

    function runSettingsTest() {
        if (backend && backend.testProviderConfig) {
            settingsTestResult = backend.testProviderConfig(settingProviderType, settingApiKey, settingBaseUrl, settingModel)
        }
    }

    function runExternalTest() {
        if (backend && backend.testExternalConfig) {
            settingsExternalTestResult = backend.testExternalConfig(settingTelegramToken, settingFeishuAppId, settingFeishuAppSecret)
        }
    }

    function loadSkillsOverview() {
        skillsListModel.clear()
        skillLoadErrorsText = ""
        if (!(backend && backend.getSkillsOverview)) {
            return
        }

        var skills = backend.getSkillsOverview()
        for (var i = 0; i < skills.length; ++i) {
            skillsListModel.append(skills[i])
        }

        if (backend && backend.getSkillLoadErrors) {
            var errs = backend.getSkillLoadErrors()
            if (errs && errs.length > 0) {
                skillLoadErrorsText = errs.join("\n")
            }
        }
    }

    function replaceListModelData(model, rows) {
        model.clear()
        if (!rows) {
            return
        }
        for (var i = 0; i < rows.length; ++i) {
            model.append(rows[i])
        }
    }

    function computeTokenSeriesMax(model) {
        var maxValue = 1
        for (var i = 0; i < model.count; ++i) {
            var row = model.get(i)
            var promptValue = row.prompt ? Number(row.prompt) : 0
            var completionValue = row.completion ? Number(row.completion) : 0
            var totalValue = row.total ? Number(row.total) : (promptValue + completionValue)
            var value = 0
            if (tokenShowPrompt) value = Math.max(value, promptValue)
            if (tokenShowCompletion) value = Math.max(value, completionValue)
            if (tokenShowTotal) value = Math.max(value, totalValue)
            if (!(tokenShowPrompt || tokenShowCompletion || tokenShowTotal)) {
                value = totalValue
            }
            if (value > maxValue) {
                maxValue = value
            }
        }
        return maxValue
    }

    function formatTokenValue(v) {
        var n = Number(v || 0)
        return n.toLocaleString(Qt.locale(), "f", 0)
    }

    function visibleDailyCount() {
        var end = tokenDailyViewEnd > tokenDailyViewStart ? tokenDailyViewEnd : tokenDailyModel.count
        var start = Math.max(0, tokenDailyViewStart)
        return Math.max(0, end - start)
    }

    function clampDailyViewWindow() {
        if (tokenDailyModel.count <= 0) {
            tokenDailyViewStart = 0
            tokenDailyViewEnd = 0
            return
        }
        if (tokenDailyViewEnd <= tokenDailyViewStart) {
            tokenDailyViewStart = 0
            tokenDailyViewEnd = tokenDailyModel.count
        }
        var span = Math.max(7, tokenDailyViewEnd - tokenDailyViewStart)
        if (span > tokenDailyModel.count) span = tokenDailyModel.count
        tokenDailyViewStart = Math.max(0, Math.min(tokenDailyViewStart, tokenDailyModel.count - span))
        tokenDailyViewEnd = tokenDailyViewStart + span
    }

    function resetDailyZoom() {
        tokenDailyViewStart = 0
        tokenDailyViewEnd = tokenDailyModel.count
    }

    function zoomDaily(factor) {
        if (tokenDailyModel.count <= 0) {
            return
        }
        clampDailyViewWindow()
        var span = tokenDailyViewEnd - tokenDailyViewStart
        var center = tokenDailyViewStart + span / 2
        var nextSpan = Math.round(span * factor)
        nextSpan = Math.max(7, Math.min(tokenDailyModel.count, nextSpan))
        var nextStart = Math.round(center - nextSpan / 2)
        nextStart = Math.max(0, Math.min(nextStart, tokenDailyModel.count - nextSpan))
        tokenDailyViewStart = nextStart
        tokenDailyViewEnd = nextStart + nextSpan
        // 重置hover状态，避免zoom后显示过时的tooltip
        tokenHoveredDailyIndex = -1
    }

    function findTokenYearIndex(yearValue) {
        var y = Number(yearValue)
        for (var i = 0; i < tokenYearOptionsModel.count; ++i) {
            if (Number(tokenYearOptionsModel.get(i).year) === y) {
                return i
            }
        }
        return -1
    }

    function findTokenMonthIndex(monthValue) {
        var m = Number(monthValue)
        for (var i = 0; i < tokenMonthOptionsModel.count; ++i) {
            if (Number(tokenMonthOptionsModel.get(i).value) === m) {
                return i
            }
        }
        return -1
    }

    function syncTokenYearSelectors() {
        if (tokenYearSelector) {
            tokenYearSelector.currentIndex = findTokenYearIndex(tokenSelectedYear)
        }
        if (tokenResetYearSelector) {
            tokenResetYearSelector.currentIndex = findTokenYearIndex(tokenResetYear)
        }
        if (tokenResetMonthSelector) {
            tokenResetMonthSelector.currentIndex = findTokenMonthIndex(tokenResetMonth)
        }
    }

    function loadTokenUsageAnalytics() {
        if (!(backend && backend.getTokenUsageAnalytics)) {
            tokenAnalyticsStatus = "Token 图表: 后端未就绪"
            replaceListModelData(tokenDailyModel, [])
            replaceListModelData(tokenMonthlyModel, [])
            replaceListModelData(tokenYearlyModel, [])
            replaceListModelData(tokenRecentEventsModel, [])
            return
        }

        var analytics = backend.getTokenUsageAnalytics(tokenChartDays, tokenSelectedYear)
        replaceListModelData(tokenDailyModel, analytics.dailySeries ? analytics.dailySeries : [])
        replaceListModelData(tokenMonthlyModel, analytics.monthlySeries ? analytics.monthlySeries : [])
        replaceListModelData(tokenYearlyModel, analytics.yearlySeries ? analytics.yearlySeries : [])
        replaceListModelData(tokenRecentEventsModel, analytics.recentEvents ? analytics.recentEvents : [])

        tokenCurrentMonthTotal = analytics.currentMonthTotal ? Number(analytics.currentMonthTotal) : 0
        tokenCurrentYearTotal = analytics.currentYearTotal ? Number(analytics.currentYearTotal) : 0
        tokenAllTimeTotal = analytics.allTimeTotal ? Number(analytics.allTimeTotal) : 0

        tokenDailyMax = computeTokenSeriesMax(tokenDailyModel)
        tokenMonthlyMax = computeTokenSeriesMax(tokenMonthlyModel)
        tokenYearlyMax = computeTokenSeriesMax(tokenYearlyModel)

        tokenYearOptionsModel.clear()
        var years = analytics.availableYears ? analytics.availableYears : []
        if (years.length === 0) {
            years = [new Date().getFullYear()]
        }

        var selectedYearExists = false
        var resetYearExists = false
        for (var i = 0; i < years.length; ++i) {
            var y = Number(years[i])
            tokenYearOptionsModel.append({ year: y, label: String(y) })
            if (y === Number(tokenSelectedYear)) {
                selectedYearExists = true
            }
            if (y === Number(tokenResetYear)) {
                resetYearExists = true
            }
        }

        if (!selectedYearExists && tokenYearOptionsModel.count > 0) {
            tokenSelectedYear = Number(tokenYearOptionsModel.get(tokenYearOptionsModel.count - 1).year)
        }
        if (!resetYearExists) {
            tokenResetYear = tokenSelectedYear
        }

        syncTokenYearSelectors()
        if (tokenDailyViewEnd <= tokenDailyViewStart || tokenDailyViewEnd > tokenDailyModel.count) {
            resetDailyZoom()
        } else {
            clampDailyViewWindow()
        }
        tokenAnalyticsStatus = "最近 " + tokenChartDays + " 天已加载，可按年查看月度统计。"
    }

    function requestTokenReset(scopeText) {
        tokenResetScope = scopeText
        if (scopeText === "all") {
            tokenResetSummary = "将重置全部 Token 统计（所有年份、月份和明细记录）。"
        } else if (scopeText === "year") {
            tokenResetSummary = "将重置 " + tokenResetYear + " 年的 Token 统计。"
        } else {
            tokenResetSummary = "将重置 " + tokenResetYear + "-" + (tokenResetMonth < 10 ? "0" + tokenResetMonth : tokenResetMonth) + " 的 Token 统计。"
        }
        tokenResetDialog.open()
    }

    function applyTokenReset() {
        if (!(backend && backend.resetTokenUsage)) {
            tokenAnalyticsStatus = "重置失败：后端未就绪"
            return
        }

        var ok = false
        if (tokenResetScope === "all") {
            ok = backend.resetTokenUsage("all", 0, 0)
        } else if (tokenResetScope === "year") {
            ok = backend.resetTokenUsage("year", Number(tokenResetYear), 0)
        } else {
            ok = backend.resetTokenUsage("month", Number(tokenResetYear), Number(tokenResetMonth))
        }

        if (ok) {
            tokenAnalyticsStatus = "重置完成，统计已刷新。"
            loadTokenStats(true)
        } else {
            tokenAnalyticsStatus = "重置失败，请检查参数。"
        }
    }

    function loadTokenStats(refreshAnalytics) {
        if (!(backend && backend.getTokenStats)) {
            tokenStatsText = "Token 统计: 后端未就绪"
            return
        }

        var stats = backend.getTokenStats()
        tokenLastPrompt = stats.lastPrompt ? stats.lastPrompt : 0
        tokenLastCompletion = stats.lastCompletion ? stats.lastCompletion : 0
        tokenLastTotal = stats.lastTotal ? stats.lastTotal : 0
        tokenAccPrompt = stats.accPrompt ? stats.accPrompt : 0
        tokenAccCompletion = stats.accCompletion ? stats.accCompletion : 0
        tokenAccTotal = stats.accTotal ? stats.accTotal : 0
        tokenCurrentMonthTotal = stats.currentMonthTotal ? Number(stats.currentMonthTotal) : tokenCurrentMonthTotal
        tokenCurrentYearTotal = stats.currentYearTotal ? Number(stats.currentYearTotal) : tokenCurrentYearTotal
        tokenAllTimeTotal = stats.allTimeTotal ? Number(stats.allTimeTotal) : tokenAllTimeTotal
        tokenStatsText = stats.usageText ? stats.usageText : "本轮消耗(API)：0 Token"

        if (refreshAnalytics === true || activePanel === "tokens") {
            loadTokenUsageAnalytics()
        }
    }

    function openRenameSessionDialog(sessionId, sessionName) {
        renameSessionId = sessionId ? sessionId : ""
        renameSessionNameField.text = sessionName ? sessionName : ""
        renameSessionDialog.open()
    }

    function loadCompressionInfo() {
        if (!(backend && backend.getCompressionInfo)) {
            compressionStatusText = "上下文压缩: 后端未就绪"
            return
        }

        var info = backend.getCompressionInfo()
        var apiTokens = info.apiTokenCount ? Number(info.apiTokenCount) : 0
        var memoryTokens = info.memoryTokenCount ? Number(info.memoryTokenCount) : 0
        compressionTokenCount = info.tokenCount ? Number(info.tokenCount) : (apiTokens > 0 ? apiTokens : memoryTokens)
        compressionThreshold = info.threshold ? info.threshold : 0
        compressionContextLimit = info.contextLimit ? info.contextLimit : 0
        compressionMessageCount = info.messageCount ? info.messageCount : 0
        compressionNeeds = info.needsCompression === true

        var base = "上下文占用(API/估算Memory): " + apiTokens + "/" + memoryTokens + " token"
        if (compressionThreshold > 0) {
            base += "，阈值 " + compressionThreshold
        }
        if (compressionContextLimit > 0) {
            base += " (模型最大上下文 " + compressionContextLimit + " × 60%)"
        }
        base += compressionNeeds ? " · 待压缩" : " · 正常"
        if (info.status && info.status.length > 0) {
            base += " · " + info.status
        }
        compressionStatusText = base
    }

    function loadLogsContent() {
        if (!(backend && backend.readWorkspaceFile)) {
            logsContent = "暂无日志。"
            return
        }

        var text = backend.readWorkspaceFile("logs/app.log")
        if (text.length === 0) {
            text = backend.readWorkspaceFile("logs/latest.log")
        }
        if (text.length === 0) {
            text = backend.readWorkspaceFile("memory/HISTORY.md")
        }
        logsContent = text.length > 0 ? text : "暂无日志。"
    }

    function loadMemoryContent() {
        if (!(backend && backend.readWorkspaceFile)) {
            memoryContent = "暂无 memory。"
            historyContent = "暂无 history。"
            memoryOverviewStatus = "memory 后端未就绪"
            return
        }

        var memoryText = backend.readWorkspaceFile("memory/MEMORY.md")
        var historyText = backend.readWorkspaceFile("memory/HISTORY.md")
        memoryContent = memoryText.length > 0 ? memoryText : "暂无 memory。"
        historyContent = historyText.length > 0 ? historyText : "暂无 history。"

        if (backend && backend.getMemoryOverview) {
            var overview = backend.getMemoryOverview()
            if (overview && overview.status === "ok") {
                memoryOverviewStatus = "memory " + (overview.memoryChars || 0) + " chars · history " + (overview.historyLines || 0) + " lines"
            } else {
                memoryOverviewStatus = "memory 数据暂不可用"
            }
        }
    }

    function saveMemoryContent() {
        if (!(backend && backend.writeWorkspaceFile)) {
            memoryOverviewStatus = "memory 保存失败：后端未就绪"
            return
        }
        var okMemory = backend.writeWorkspaceFile("memory/MEMORY.md", memoryContent)
        var okHistory = backend.writeWorkspaceFile("memory/HISTORY.md", historyContent)
        memoryOverviewStatus = (okMemory && okHistory) ? "memory/history 已保存" : "保存失败，请检查文件权限"
    }

    function appendManualMemoryNote(noteText) {
        if (!(backend && backend.appendMemoryNote)) {
            memoryOverviewStatus = "追加记忆失败：后端不可用"
            return
        }
        var ok = backend.appendMemoryNote(noteText)
        memoryOverviewStatus = ok ? "记忆已追加到 MEMORY.md" : "追加记忆失败"
        if (ok) {
            loadMemoryContent()
        }
    }

    function runMemorySearch() {
        memorySearchModel.clear()
        if (!(backend && backend.searchMemory)) {
            return
        }
        var rows = backend.searchMemory(memorySearchQuery, 8)
        for (var i = 0; i < rows.length; ++i) {
            memorySearchModel.append({ text: rows[i] })
        }
    }

    function openPanel(panelName) {
        if (activePanel === "settings" && panelName !== "settings") {
            saveSettingsContent()
        }
        activePanel = panelName
        if (panelName === "settings") {
            loadSettingsContent()
        } else if (panelName === "logs") {
            loadLogsContent()
        } else if (panelName === "memory") {
            loadMemoryContent()
        } else if (panelName === "tokens") {
            loadTokenStats()
        }
        loadCompressionInfo()
    }

    function toggleSessionSelection(sessionId, selected) {
        var sid = sessionId ? String(sessionId) : ""
        if (sid.length === 0) return
        var next = []
        var exists = false
        for (var i = 0; i < selectedSessionIds.length; ++i) {
            var cur = String(selectedSessionIds[i])
            if (cur === sid) {
                exists = true
            } else {
                next.push(cur)
            }
        }
        if (selected) {
            if (!exists) next.push(sid)
        }
        selectedSessionIds = next
    }

    function isSessionSelected(sessionId) {
        var sid = sessionId ? String(sessionId) : ""
        for (var i = 0; i < selectedSessionIds.length; ++i) {
            if (String(selectedSessionIds[i]) === sid) return true
        }
        return false
    }

    onActivePanelChanged: {
        panelSwitchOpacity = 0.86
        panelSwitchAnim.restart()
    }

    onTokenSelectedYearChanged: {
        if (root.activePanel === "tokens") {
            root.loadTokenUsageAnalytics()
        }
    }

    onTokenChartDaysChanged: {
        if (root.activePanel === "tokens") {
            root.loadTokenUsageAnalytics()
        }
    }

    onTokenShowPromptChanged: {
        tokenDailyMax = computeTokenSeriesMax(tokenDailyModel)
        tokenMonthlyMax = computeTokenSeriesMax(tokenMonthlyModel)
        tokenYearlyMax = computeTokenSeriesMax(tokenYearlyModel)
    }

    onTokenShowCompletionChanged: {
        tokenDailyMax = computeTokenSeriesMax(tokenDailyModel)
        tokenMonthlyMax = computeTokenSeriesMax(tokenMonthlyModel)
        tokenYearlyMax = computeTokenSeriesMax(tokenYearlyModel)
    }

    onTokenShowTotalChanged: {
        tokenDailyMax = computeTokenSeriesMax(tokenDailyModel)
        tokenMonthlyMax = computeTokenSeriesMax(tokenMonthlyModel)
        tokenYearlyMax = computeTokenSeriesMax(tokenYearlyModel)
    }

    NumberAnimation {
        id: panelSwitchAnim
        target: root
        property: "panelSwitchOpacity"
        from: 0.86
        to: 1.0
        duration: 170
        easing.type: Easing.OutCubic
    }

    Component.onCompleted: {
        if (backend && backend.refreshSessions) {
            backend.refreshSessions()
        }
        loadSettingsContent()
        loadLogsContent()
        loadMemoryContent()
        loadTokenStats()
        loadCompressionInfo()
    }

    onClosing: {
        saveSettingsContent()
    }

    Timer {
        id: settingsSaveTimer
        interval: 600
        repeat: false
        onTriggered: saveSettingsContent()
    }

    Timer {
        interval: 3000
        running: true
        repeat: true
        onTriggered: {
            root.loadCompressionInfo()
            root.loadTokenStats()
        }
    }

    Timer {
        id: generatingPulse
        interval: 350
        running: backend ? backend.generating : false
        repeat: true
        onTriggered: {
            root.generatingDots = (root.generatingDots + 1) % 4
        }
    }

    component GlassDialog: Dialog {
        id: glassDlg
        modal: true
        padding: 20
        topPadding: 20
        bottomPadding: 0

        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)

        header: Rectangle {
            implicitHeight: 36
            radius: 18
            color: Qt.rgba(252/255, 252/255, 255/255, 0.92)
            border.color: Qt.rgba(190/255, 190/255, 205/255, 0.25)
            border.width: 0.5

            Rectangle {
                anchors {
                    top: parent.top
                    horizontalCenter: parent.horizontalCenter
                }
                width: parent.width * 0.55
                height: 0.5
                radius: 0.25
                color: Qt.rgba(1, 1, 1, 0.50)
            }

            Rectangle {
                anchors {
                    left: parent.left
                    leftMargin: 0.5
                    right: parent.right
                    rightMargin: 0.5
                    bottom: parent.bottom
                }
                height: 0.5
                radius: 0.25
                color: Qt.rgba(190/255, 190/255, 205/255, 0.20)
            }

            Text {
                anchors.centerIn: parent
                text: glassDlg.title
                font.pixelSize: 13
                font.weight: Font.SemiBold
                color: root.textPrimary
            }
        }

        background: Rectangle {
            radius: 18
            clip: true
            color: Qt.rgba(252/255, 252/255, 255/255, 0.96)
            border.color: Qt.rgba(190/255, 190/255, 205/255, 0.30)
            border.width: 0.5

            Rectangle {
                anchors {
                    top: parent.top
                    horizontalCenter: parent.horizontalCenter
                }
                width: parent.width * 0.55
                height: 0.5
                radius: 0.25
                color: Qt.rgba(1, 1, 1, 0.50)
            }
        }

        footer: RowLayout {
            spacing: 12

            Item { Layout.fillWidth: true }

            Button {
                id: dlgCancelBtn
                text: "取消"
                implicitWidth: 80
                implicitHeight: 32
                font.pixelSize: 12
                font.weight: Font.Medium
                hoverEnabled: true

                contentItem: Text {
                    text: dlgCancelBtn.text
                    font: dlgCancelBtn.font
                    color: root.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 16
                    color: dlgCancelBtn.pressed
                        ? Qt.rgba(210/255, 210/255, 225/255, 0.52)
                        : (dlgCancelBtn.hovered
                            ? Qt.rgba(225/255, 225/255, 240/255, 0.48)
                            : Qt.rgba(240/255, 240/255, 250/255, 0.38))
                    border.color: Qt.rgba(1, 1, 1, 0.55)
                    border.width: 0.5

                    Rectangle {
                        anchors {
                            top: parent.top
                            horizontalCenter: parent.horizontalCenter
                        }
                        width: parent.width * 0.55
                        height: 0.5
                        radius: 0.25
                        color: Qt.rgba(1, 1, 1, 0.50)
                    }

                    Behavior on color { ColorAnimation { duration: 120 } }
                }

                onClicked: glassDlg.reject()
            }

            Button {
                id: dlgAcceptBtn
                text: "确定"
                implicitWidth: 80
                implicitHeight: 32
                font.pixelSize: 12
                font.weight: Font.Medium
                hoverEnabled: true

                contentItem: Text {
                    text: dlgAcceptBtn.text
                    font: dlgAcceptBtn.font
                    color: "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 16
                    color: dlgAcceptBtn.pressed
                        ? Qt.rgba(0, 100/255, 220/255, 0.92)
                        : (dlgAcceptBtn.hovered
                            ? Qt.rgba(0, 110/255, 235/255, 0.88)
                            : Qt.rgba(0, 122/255, 255/255, 0.82))
                    border.color: Qt.rgba(1, 1, 1, 0.45)
                    border.width: 0.5

                    Rectangle {
                        anchors {
                            top: parent.top
                            horizontalCenter: parent.horizontalCenter
                        }
                        width: parent.width * 0.55
                        height: 0.5
                        radius: 0.25
                        color: Qt.rgba(1, 1, 1, 0.42)
                    }

                    Behavior on color { ColorAnimation { duration: 120 } }
                }

                onClicked: glassDlg.accept()
            }

            Item { Layout.fillWidth: true }
        }
    }

    component GlassButton: Button {
        id: btn
        hoverEnabled: true
        font.pixelSize: 13
        palette.buttonText: root.textPrimary
        leftPadding: 14
        rightPadding: 14
        topPadding: 8
        bottomPadding: 8

        background: Rectangle {
            radius: 10
            border.color: btn.hovered ? Qt.rgba(160/255, 160/255, 180/255, 0.45) : Qt.rgba(190/255, 190/255, 205/255, 0.25)
            border.width: 0.5
            color: btn.down ? Qt.rgba(210/255, 210/255, 225/255, 0.40) : (btn.hovered ? Qt.rgba(225/255, 225/255, 240/255, 0.35) : Qt.rgba(240/255, 240/255, 248/255, 0.22))
        }
    }

    component NavButton: Button {
        id: navBtn
        required property string panelKey
        required property string labelText

        hoverEnabled: true
        font.pixelSize: 13
        font.weight: root.activePanel === panelKey ? Font.DemiBold : Font.Medium
        leftPadding: 14
        rightPadding: 14
        topPadding: 10
        bottomPadding: 10
        text: labelText
        palette.buttonText: root.activePanel === panelKey ? root.accent : root.textPrimary

        scale: navBtn.down ? 0.96 : 1.0
        Behavior on scale {
            NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
        }

        background: Rectangle {
            radius: 11
            color: root.activePanel === navBtn.panelKey
                ? Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.14)
                : (navBtn.hovered ? Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.35) : Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.20))
            border.color: root.activePanel === navBtn.panelKey
                ? Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.32)
                : Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)
                
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        onClicked: root.openPanel(panelKey)
    }

    component Expander: Column {
        id: expanderRoot
        property Component contentItem: null
        property bool expanded: false
        spacing: 4

        Button {
            text: expanderRoot.expanded ? "▲ 隐藏详情" : "▼ 展开详情"
            font.pixelSize: 10
            background: null
            onClicked: expanderRoot.expanded = !expanderRoot.expanded
            contentItem: Text {
                text: parent.text
                color: root.accent
                font.pixelSize: parent.font.pixelSize
            }
        }

        Loader {
            id: contentLoader
            width: parent.width
            visible: expanderRoot.expanded
            sourceComponent: expanderRoot.contentItem
        }
    }

    component DangerButton: Button {
        id: dangerBtn
        hoverEnabled: true
        font.pixelSize: 12
        font.weight: Font.DemiBold
        palette.buttonText: "#B42318"
        leftPadding: 14
        rightPadding: 14
        topPadding: 8
        bottomPadding: 8

        scale: dangerBtn.down ? 0.96 : (dangerBtn.hovered ? 1.02 : 1.0)
        Behavior on scale {
            NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
        }

        background: Rectangle {
            radius: 11
            border.color: Qt.rgba(180 / 255, 35 / 255, 24 / 255, 0.28)
            gradient: Gradient {
                GradientStop { position: 0.0; color: dangerBtn.down ? Qt.rgba(1, 214 / 255, 211 / 255, 0.88) : (dangerBtn.hovered ? Qt.rgba(1, 239 / 255, 237 / 255, 0.9) : Qt.rgba(1, 248 / 255, 247 / 255, 0.86)) }
                GradientStop { position: 1.0; color: dangerBtn.down ? Qt.rgba(253 / 255, 206 / 255, 200 / 255, 0.74) : (dangerBtn.hovered ? Qt.rgba(253 / 255, 221 / 255, 216 / 255, 0.7) : Qt.rgba(247 / 255, 233 / 255, 230 / 255, 0.62)) }
            }
        }
    }

    component GlassTextField: TextField {
        id: glassField
        font.pixelSize: 12
        leftPadding: 12
        rightPadding: 12
        topPadding: 8
        bottomPadding: 8

        background: Rectangle {
            radius: 12
            border.color: glassField.activeFocus
                ? Qt.rgba(0/255, 122/255, 255/255, 0.45)
                : Qt.rgba(195/255, 198/255, 212/255, 0.30)
            border.width: glassField.activeFocus ? 1 : 0.5
            color: Qt.rgba(245/255, 245/255, 250/255, 0.40)

            Behavior on border.color {
                ColorAnimation { duration: 150 }
            }
            Behavior on border.width {
                NumberAnimation { duration: 100 }
            }
        }
    }

    component GlassComboBox: ComboBox {
        id: glassCombo
        font.pixelSize: 12

        background: Rectangle {
            radius: 12
            color: Qt.rgba(245/255, 245/255, 250/255, 0.40)
            border.color: glassCombo.hovered
                ? Qt.rgba(160/255, 160/255, 180/255, 0.50)
                : Qt.rgba(190/255, 190/255, 205/255, 0.30)
            border.width: glassCombo.hovered ? 1 : 0.5

            Behavior on border.color {
                ColorAnimation { duration: 150 }
            }
            Behavior on border.width {
                NumberAnimation { duration: 100 }
            }
        }

        popup: Popup {
            y: glassCombo.height + 4
            width: glassCombo.width
            implicitHeight: contentItem.implicitHeight
            padding: 6

            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: glassCombo.popup.visible ? glassCombo.delegateModel : null
                currentIndex: glassCombo.highlightedIndex

                ScrollIndicator.vertical: ScrollIndicator {}
            }

            background: Rectangle {
                radius: 14
                color: Qt.rgba(252/255, 252/255, 255/255, 0.97)
                border.color: Qt.rgba(190/255, 190/255, 205/255, 0.30)
                border.width: 0.5
            }
        }
    }

    component GlassMenu: Menu {
        id: glassMenu
        
        background: Rectangle {
            implicitWidth: 160
            radius: 12
            color: Qt.rgba(250/255, 250/255, 255/255, 0.92)
            border.color: Qt.rgba(190/255, 190/255, 205/255, 0.35)
        }
        
        delegate: MenuItem {
            id: menuItem
            implicitHeight: 36
            
            background: Rectangle {
                radius: 6
                color: menuItem.highlighted ? Qt.rgba(0/255, 122/255, 255/255, 0.10) : "transparent"
                anchors.fill: parent
                anchors.margins: 4
                
                Behavior on color {
                    ColorAnimation { duration: 150 }
                }
            }
            
            contentItem: Text {
                text: menuItem.text
                color: menuItem.highlighted ? "#0066FF" : root.textPrimary
                font.pixelSize: 12
                font.weight: menuItem.highlighted ? Font.DemiBold : Font.Normal
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                
                Behavior on color {
                    ColorAnimation { duration: 150 }
                }
            }
        }
        
        enter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 150; easing.type: Easing.OutQuad }
                NumberAnimation { property: "scale"; from: 0.95; to: 1.0; duration: 150; easing.type: Easing.OutBack }
            }
        }
        
        exit: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100; easing.type: Easing.InQuad }
                NumberAnimation { property: "scale"; from: 1.0; to: 0.95; duration: 100; easing.type: Easing.InQuad }
            }
        }
    }

    Item {
        id: backgroundContainer
        anchors.fill: parent
        visible: root.settingBackgroundBlurRadius === 0

        Rectangle {
            id: backgroundLayer
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: root.bgTop }
                GradientStop { position: 1.0; color: root.bgBottom }
            }
        }

        Image {
            id: customBackgroundImage
            anchors.fill: parent
            source: cachedBgPath.length > 0 ? root.normalizedFileUrl(cachedBgPath) : ""
            fillMode: Image.PreserveAspectCrop
            smooth: true
            asynchronous: false
            cache: true
            visible: status === Image.Ready && source.toString().length > 0
            opacity: visible ? 1.0 : 0.0
        }
    }

    property string cachedBgPath: ""
    onSettingBackgroundImagePathChanged: {
        if (settingBackgroundImagePath.length > 0) {
            cachedBgPath = settingBackgroundImagePath
        } else {
            cachedBgPath = ""
        }
    }

    ShaderEffectSource {
        id: backgroundSource
        anchors.fill: parent
        sourceItem: backgroundContainer
        visible: false
    }

    MultiEffect {
        id: backgroundBlur
        anchors.fill: parent
        source: backgroundSource
        blurEnabled: root.settingBackgroundBlurRadius > 0
        blurMax: 64
        blur: Math.min(1.0, root.settingBackgroundBlurRadius / 36.0)
        visible: root.settingBackgroundBlurRadius > 0
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 326
            Layout.fillHeight: true
            radius: 24
            color: root.panelFill
            border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 88
                    radius: 16
                    color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.28)
                    border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.22)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 4

                        RowLayout {
                            spacing: 8
                            Image {
                                source: "qrc:/app/icon.png"
                                sourceSize.width: 44
                                sourceSize.height: 44
                                width: 44
                                height: 44
                                fillMode: Image.PreserveAspectFit
                            }
                            Text {
                                text: "Claw++"
                                color: root.textPrimary
                                font.pixelSize: 28
                                font.bold: true
                            }
                        }

                        Text {
                            text: "Win11 Glass Command Center"
                            color: root.textSecondary
                            font.pixelSize: 13
                        }
                    }
                }

                Rectangle {
                    id: navBlock
                    Layout.fillWidth: true
                    Layout.preferredHeight: navColumn.implicitHeight + 20
                    Layout.minimumHeight: navColumn.implicitHeight + 20
                    radius: 16
                    color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.22)
                    border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)
                    clip: true

                    ColumnLayout {
                        id: navColumn
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Text {
                            text: "功能导航"
                            color: root.accent
                            font.pixelSize: 11
                            font.bold: true
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 1.1
                        }

                        NavButton {
                            Layout.fillWidth: true
                            panelKey: "chat"
                            labelText: "聊天"
                        }

                        NavButton {
                            Layout.fillWidth: true
                            panelKey: "settings"
                            labelText: "设置"
                        }

                        NavButton {
                            Layout.fillWidth: true
                            panelKey: "logs"
                            labelText: "日志"
                        }

                        NavButton {
                            Layout.fillWidth: true
                            panelKey: "memory"
                            labelText: "Memory"
                        }

                        NavButton {
                            Layout.fillWidth: true
                            panelKey: "tokens"
                            labelText: "Token 统计"
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 16
                    color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.18)
                    border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.15)
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: "会话列表"
                                color: root.accent
                                font.pixelSize: 11
                                font.bold: true
                                font.capitalization: Font.AllUppercase
                                font.letterSpacing: 1.1
                            }

                            Item { Layout.fillWidth: true }

                            Button {
                                text: "+"
                                implicitWidth: 26
                                implicitHeight: 26
                                font.pixelSize: 14
                                onClicked: newSessionDialog.open()
                                background: Rectangle {
                                    radius: 8
                                    color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.13)
                                    border.color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.28)
                                }
                            }

                            Button {
                                text: "批删"
                                implicitHeight: 26
                                implicitWidth: 46
                                visible: root.selectedSessionIds.length > 0
                                onClicked: batchDeleteDialog.open()
                                background: Rectangle {
                                    radius: 8
                                    color: Qt.rgba(180/255, 35/255, 24/255, 0.1)
                                    border.color: Qt.rgba(180/255, 35/255, 24/255, 0.28)
                                }
                            }
                        }

                        ListView {
                            id: sessionList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 6
                            clip: true
                            model: backend && backend.sessionsModel ? backend.sessionsModel : demoSessions

                            add: Transition {
                                ParallelAnimation {
                                    NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 250; easing.type: Easing.OutQuad }
                                    NumberAnimation { property: "scale"; from: 0.95; to: 1.0; duration: 250; easing.type: Easing.OutBack }
                                }
                            }
                            
                            addDisplaced: Transition {
                                NumberAnimation { properties: "y"; duration: 200; easing.type: Easing.OutCubic }
                            }

                            delegate: Rectangle {
                                required property int index
                                property var rowData: sessionList.model && sessionList.model.get
                                                      ? sessionList.model.get(index)
                                                      : ({})
                                property string sessionIdValue: sessionList.model && sessionList.model.sessionIdAt
                                                                ? sessionList.model.sessionIdAt(index)
                                                                : (rowData.id ? rowData.id : (rowData.idValue ? rowData.idValue : ""))
                                property bool selectedValue: rowData.selected === true

                                width: sessionList.width
                                height: 56
                                radius: 14
                                color: selectedValue ? root.accentSoft : Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.22)
                                border.color: selectedValue ? Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.34) : Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)
                                
                                Behavior on color { ColorAnimation { duration: 150 } }
                                Behavior on border.color { ColorAnimation { duration: 150 } }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    CheckBox {
                                        checked: root.isSessionSelected(sessionIdValue)
                                        onToggled: root.toggleSessionSelection(sessionIdValue, checked)
                                    }

                                    Rectangle {
                                        width: 8
                                        height: 8
                                        radius: 4
                                        color: selectedValue ? root.accent : Qt.rgba(96 / 255, 112 / 255, 137 / 255, 0.55)
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Text {
                                            text: rowData.name ? rowData.name : "未命名会话"
                                            color: root.textPrimary
                                            font.pixelSize: 12
                                            font.bold: selectedValue
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            text: (rowData.statusText ? rowData.statusText : "active")
                                                  + (rowData.updatedAt ? " · " + rowData.updatedAt : "")
                                            color: root.textSecondary
                                            font.pixelSize: 10
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Rectangle {
                                        visible: rowData.pinned === true
                                        width: 44
                                        height: 22
                                        radius: 11
                                        color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.12)
                                        border.color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.26)

                                        Text {
                                            anchors.centerIn: parent
                                            text: "置顶"
                                            color: root.accent
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.LeftButton
                                    onClicked: {
                                        if (!backend) {
                                            return
                                        }
                                        var sid = sessionIdValue
                                        if (sid.length > 0) {
                                            backend.selectSession(sid)
                                        }
                                    }
                                }

                                GlassMenu {
                                    id: sessionItemMenu

                                    MenuItem {
                                        text: "重命名"
                                        onTriggered: {
                                            if (sessionIdValue.length > 0) {
                                                root.openRenameSessionDialog(sessionIdValue, rowData.name ? rowData.name : "")
                                            }
                                        }
                                    }

                                    MenuItem {
                                        text: rowData.pinned === true ? "取消置顶" : "置顶"
                                        onTriggered: {
                                            if (backend && sessionIdValue.length > 0) {
                                                backend.togglePinSessionById(sessionIdValue)
                                            }
                                        }
                                    }

                                    MenuItem {
                                        text: "删除会话"
                                        onTriggered: {
                                            if (backend && sessionIdValue.length > 0) {
                                                backend.deleteSessionById(sessionIdValue)
                                            }
                                        }
                                    }

                                    MenuSeparator {}

                                    MenuItem {
                                        text: "批量删除未置顶"
                                        onTriggered: {
                                            if (backend && backend.deleteUnpinnedSessions) {
                                                backend.deleteUnpinnedSessions()
                                            }
                                        }
                                    }
                                }

                                TapHandler {
                                    acceptedButtons: Qt.RightButton
                                    onTapped: function(point) {
                                        if (sessionList.currentItem) {
                                            sessionList.currentItem.forceActiveFocus()
                                        }
                                        sessionItemMenu.popup()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 24
            color: root.panelFill
            border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 112
                    radius: 16
                    color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.28)
                    border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.22)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3

                            Text {
                                text: root.activePanel === "chat" ? (backend ? backend.currentSessionName : "消息工作区")
                                                                   : (root.activePanel === "settings" ? "设置中心"
                                                                        : (root.activePanel === "logs" ? "日志中心"
                                                                            : (root.activePanel === "memory" ? "Memory 面板" : "Token 统计")))
                                color: root.textPrimary
                                font.pixelSize: 19
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: root.activePanel === "chat" ? (backend ? backend.statusText : "Design Studio 预览模式")
                                                                   : "应用内功能面板"
                                color: root.textSecondary
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }

                            Text {
                                text: backend ? backend.usageText : "本轮消耗(API)：0 Token"
                                color: root.textSecondary
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                RowLayout {
                                    spacing: 6
                                    Layout.fillWidth: true

                                    Text {
                                        text: root.compressionStatusText.length > 0
                                            ? root.compressionStatusText
                                            : "上下文压缩: --"
                                        color: root.compressionNeeds ? "#B42318" : root.textSecondary
                                        font.pixelSize: 10
                                    }

                                    Item { Layout.fillWidth: true }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: 6
                                    radius: 3
                                    color: Qt.rgba(0, 0, 0, 0.1)
                                    border.color: Qt.rgba(0, 0, 0, 0.05)
                                    visible: root.compressionThreshold > 0

                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        radius: 3
                                        color: root.compressionNeeds ? "#B42318" : "#10B981"
                                        width: parent.width * Math.min(1.0, Math.max(0, root.compressionTokenCount / root.compressionThreshold))
                                    }
                                }

                                Text {
                                    text: root.compressionThreshold > 0
                                        ? (Math.round(root.compressionTokenCount / root.compressionThreshold * 100) + "% · " + root.compressionTokenCount + "/" + root.compressionThreshold + " tokens")
                                        : ""
                                    color: root.textSecondary
                                    font.pixelSize: 9
                                    visible: root.compressionThreshold > 0
                                }
                            }
                        }

                        RowLayout {
                            spacing: 8

                            Rectangle {
                                implicitWidth: 110
                                implicitHeight: 32
                                radius: 10
                                color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.12)
                                border.color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.28)

                                Text {
                                    anchors.centerIn: parent
                                    text: "面板: " + (root.activePanel === "chat" ? "聊天" : (root.activePanel === "settings" ? "设置" : (root.activePanel === "logs" ? "日志" : (root.activePanel === "memory" ? "Memory" : "Token"))))
                                    color: root.accent
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            Rectangle {
                                implicitWidth: 150
                                implicitHeight: 32
                                radius: 10
                                color: Qt.rgba(1, 1, 1, 0.55)
                                border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                Text {
                                    anchors.centerIn: parent
                                    text: "消息数: " + root.compressionMessageCount
                                    color: root.textSecondary
                                    font.pixelSize: 11
                                }
                            }
                        }

                        GlassButton {
                            text: "停止"
                            visible: root.activePanel === "chat"
                            enabled: backend ? backend.generating : false
                            onClicked: {
                                if (backend) {
                                    backend.stopGeneration()
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 58
                    radius: 14
                    color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.22)
                    border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        ListView {
                            id: sessionTabs
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            orientation: ListView.Horizontal
                            spacing: 6
                            clip: true
                            model: backend && backend.sessionsModel ? backend.sessionsModel : demoSessions
                            
                            add: Transition {
                                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 250; easing.type: Easing.OutQuad }
                            }
                            
                            addDisplaced: Transition {
                                NumberAnimation { properties: "x"; duration: 200; easing.type: Easing.OutCubic }
                            }

                            delegate: Button {
                                required property int index
                                property var rowData: sessionTabs.model && sessionTabs.model.get
                                                      ? sessionTabs.model.get(index)
                                                      : ({})
                                property bool selectedValue: rowData.selected === true

                                text: (rowData.name && rowData.name.length > 0)
                                    ? rowData.name
                                    : ((sessionTabs.model && sessionTabs.model.sessionIdAt)
                                       ? sessionTabs.model.sessionIdAt(index)
                                       : "会话")
                                width: Math.max(140, Math.min(260, implicitContentWidth + 36))
                                height: 40
                                font.pixelSize: 12
                                font.weight: selectedValue ? Font.DemiBold : Font.Medium
                                palette.buttonText: selectedValue ? root.accent : root.textPrimary

                                background: Rectangle {
                                    radius: 10
                                    color: selectedValue ? root.accentSoft : Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.25)
                                    border.color: selectedValue ? Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.34) : Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)
                                    
                                    Behavior on color { ColorAnimation { duration: 150 } }
                                    Behavior on border.color { ColorAnimation { duration: 150 } }
                                }

                                GlassMenu {
                                    id: tabMenu

                                    MenuItem {
                                        text: "重命名"
                                        onTriggered: {
                                            var sidRename = sessionTabs.model && sessionTabs.model.sessionIdAt
                                                ? sessionTabs.model.sessionIdAt(index)
                                                : ""
                                            if (sidRename.length > 0) {
                                                root.openRenameSessionDialog(sidRename, rowData.name ? rowData.name : "")
                                            }
                                        }
                                    }

                                    MenuItem {
                                        text: "置顶/取消"
                                        onTriggered: {
                                            var sidPin = sessionTabs.model && sessionTabs.model.sessionIdAt
                                                ? sessionTabs.model.sessionIdAt(index)
                                                : ""
                                            if (backend && sidPin.length > 0) {
                                                backend.togglePinSessionById(sidPin)
                                            }
                                        }
                                    }

                                    MenuItem {
                                        text: "删除会话"
                                        onTriggered: {
                                            if (!backend) {
                                                return
                                            }
                                            var sid = sessionTabs.model && sessionTabs.model.sessionIdAt
                                                ? sessionTabs.model.sessionIdAt(index)
                                                : (rowData.id ? rowData.id : (rowData.idValue ? rowData.idValue : ""))
                                            if (sid.length > 0) {
                                                backend.selectSession(sid)
                                                backend.deleteCurrentSession()
                                            }
                                        }
                                    }
                                }

                                TapHandler {
                                    acceptedButtons: Qt.RightButton
                                    onTapped: function(point) {
                                        if (sessionTabs.currentItem) {
                                            sessionTabs.currentItem.forceActiveFocus()
                                        }
                                        tabMenu.popup()
                                    }
                                }

                                onClicked: {
                                    if (!backend) {
                                        return
                                    }
                                    var sid = sessionTabs.model && sessionTabs.model.sessionIdAt
                                        ? sessionTabs.model.sessionIdAt(index)
                                        : (rowData.id ? rowData.id : (rowData.idValue ? rowData.idValue : ""))
                                    if (sid.length > 0) {
                                        backend.selectSession(sid)
                                    }
                                }
                            }
                        }

                    }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    opacity: root.panelSwitchOpacity
                    currentIndex: root.activePanel === "chat" ? 0
                        : (root.activePanel === "settings" ? 1
                            : (root.activePanel === "logs" ? 2
                                : (root.activePanel === "memory" ? 3 : 4)))

                    Item {
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 16
                                color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.18)
                                border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.15)

                                ListView {
                                    id: chatList
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 8
                                    clip: true
                                    reuseItems: true
                                    cacheBuffer: 320
                                    model: backend && backend.messagesModel ? backend.messagesModel : demoMessages

                                    add: Transition {
                                        ParallelAnimation {
                                            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 250; easing.type: Easing.OutQuad }
                                            NumberAnimation { property: "scale"; from: 0.95; to: 1.0; duration: 250; easing.type: Easing.OutBack }
                                        }
                                    }
                                    
                                    addDisplaced: Transition {
                                        NumberAnimation { properties: "x,y"; duration: 200; easing.type: Easing.OutCubic }
                                    }

                                    footer: Item {
                                        width: chatList.width
                                        height: 40
                                        visible: root.activePanel === "chat" && (backend ? backend.generating : false)
                                        
                                        Row {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 12
                                            anchors.verticalCenter: parent.verticalCenter
                                            spacing: 6
                                            
                                            Rectangle {
                                                width: 8; height: 8; radius: 4
                                                color: "#0066FF"
                                                opacity: 0.5
                                                SequentialAnimation on opacity {
                                                    loops: Animation.Infinite
                                                    NumberAnimation { to: 1.0; duration: 400; easing.type: Easing.InOutSine }
                                                    NumberAnimation { to: 0.5; duration: 400; easing.type: Easing.InOutSine }
                                                }
                                            }
                                            Rectangle {
                                                width: 8; height: 8; radius: 4
                                                color: "#0066FF"
                                                opacity: 0.5
                                                SequentialAnimation on opacity {
                                                    loops: Animation.Infinite
                                                    PauseAnimation { duration: 150 }
                                                    NumberAnimation { to: 1.0; duration: 400; easing.type: Easing.InOutSine }
                                                    NumberAnimation { to: 0.5; duration: 400; easing.type: Easing.InOutSine }
                                                }
                                            }
                                            Rectangle {
                                                width: 8; height: 8; radius: 4
                                                color: "#0066FF"
                                                opacity: 0.5
                                                SequentialAnimation on opacity {
                                                    loops: Animation.Infinite
                                                    PauseAnimation { duration: 300 }
                                                    NumberAnimation { to: 1.0; duration: 400; easing.type: Easing.InOutSine }
                                                    NumberAnimation { to: 0.5; duration: 400; easing.type: Easing.InOutSine }
                                                }
                                            }
                                            
                                            Text {
                                                text: "AI is thinking..."
                                                color: root.textSecondary
                                                font.pixelSize: 11
                                                font.italic: true
                                                anchors.verticalCenter: parent.verticalCenter
                                            }

                                            Text {
                                                visible: backend && backend.toolHintText && backend.toolHintText.length > 0
                                                text: backend ? backend.toolHintText : ""
                                                color: root.textSecondary
                                                font.pixelSize: 10
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                        }
                                    }

                                    onCountChanged: {
                                        root.maybeScrollChatToEnd(false)
                                        root.loadCompressionInfo()
                                    }

                                    onContentHeightChanged: {
                                        if (root.chatAutoScrollEnabled) {
                                            root.maybeScrollChatToEnd(false)
                                        }
                                    }

                                    onMovementStarted: {
                                        if (chatList.draggingVertically || chatList.flicking) {
                                            root.chatAutoScrollEnabled = false
                                            root.chatScrollPending = false
                                            chatScrollTimer.stop()
                                        }
                                    }

                                    onMovementEnded: {
                                        // 手动滚动模式：不在滚动结束时自动恢复吸底
                                    }

                                    onContentYChanged: {
                                        if (chatList.atYEnd && !chatList.draggingVertically && !chatList.flicking) {
                                            root.chatAutoScrollEnabled = true
                                        }
                                    }

                                    delegate: Item {
                                        property bool userMessage: !!((typeof isUser !== "undefined" && isUser === true)
                                            || (typeof sender !== "undefined" && sender === "user"))
                                        property bool systemMessage: !!(typeof sender !== "undefined" && sender === "system")
                                        property string rawText: ((typeof displayContent !== "undefined" && displayContent !== null && String(displayContent).length > 0)
                                            ? String(displayContent)
                                            : ((typeof content !== "undefined" && content !== null) ? String(content) : ""))
                                        property bool toolCardMessage: systemMessage && rawText.indexOf("ToolCard|") === 0
                                        property string toolCardPayload: toolCardMessage ? rawText.substring("ToolCard|".length) : ""
                                        property int toolCardSep1: toolCardPayload.indexOf("|")
                                        property string toolCardKind: toolCardMessage && toolCardSep1 >= 0 ? toolCardPayload.substring(0, toolCardSep1) : ""
                                        property string toolCardRest: toolCardMessage && toolCardSep1 >= 0 ? toolCardPayload.substring(toolCardSep1 + 1) : ""
                                        property int toolCardSep2: toolCardRest.indexOf("|")
                                        property string toolCardName: toolCardMessage && toolCardSep2 >= 0 ? toolCardRest.substring(0, toolCardSep2) : toolCardRest
                                        property string toolCardTail: toolCardMessage && toolCardSep2 >= 0 ? toolCardRest.substring(toolCardSep2 + 1) : ""
                                        property int toolCardLineBreak: toolCardTail.indexOf("\n")
                                        property int toolCardActionLineBreak: toolCardTail.indexOf("\n")
                                        property string toolCardActionStatus: (toolCardMessage && toolCardKind === "Action")
                                            ? (toolCardActionLineBreak >= 0 ? toolCardTail.substring(0, toolCardActionLineBreak) : toolCardTail)
                                            : ""
                                        property string toolCardStatus: toolCardMessage && toolCardKind === "Observation" && toolCardLineBreak >= 0 ? toolCardTail.substring(0, toolCardLineBreak) : ""
                                        property string toolCardBody: toolCardMessage
                                            ? (toolCardKind === "Action"
                                                ? (toolCardActionLineBreak >= 0 ? toolCardTail.substring(toolCardActionLineBreak + 1) : "")
                                                : (toolCardLineBreak >= 0 ? toolCardTail.substring(toolCardLineBreak + 1) : toolCardTail))
                                            : ""
                                        property bool thinkingMessage: systemMessage && rawText.indexOf("Thinking:") === 0
                                        property bool actionMessage: systemMessage && rawText.indexOf("Action:") === 0
                                        property bool observationMessage: systemMessage && rawText.indexOf("Observation(") === 0
                                        property bool observationError: observationMessage && rawText.indexOf("Observation(ERR)") === 0
                                        property string metricText: rawText
                                            .replace(/`[^`]*`/g, "X")
                                            .replace(/\[[^\]]*\]\([^)]+\)/g, "X")
                                            .replace(/\s+/g, " ")
                                            .trim()
                                        property bool hasLineBreaks: rawText.indexOf("\n") >= 0
                                        property bool preExecCard: toolCardMessage && toolCardKind === "PreExec"
                                        property bool compactToolMessage: thinkingMessage || actionMessage || observationMessage
                                            || (systemMessage && (rawText.indexOf("工具") === 0
                                                || rawText.indexOf("Shell 风险评估") === 0
                                                || rawText.indexOf("子 Agent") === 0))
                                        property bool workflowLikeCard: toolCardMessage && (toolCardName.indexOf("workflow_") === 0 || toolCardName.indexOf("subagent_") === 0)
                                        property bool toolCardExpanded: workflowLikeCard
                                        property string senderLabel: userMessage ? "我" : (systemMessage ? "系统" : (backend ? backend.currentModelName : "assistant"))
                                        property string messageTime: (typeof timestamp !== "undefined" && timestamp)
                                            ? String(timestamp).replace("T", " ").slice(0, 16)
                                            : ""
                                        property string segmentTitle: thinkingMessage ? "Thinking"
                                            : (actionMessage ? "Action"
                                                : (observationMessage ? (observationError ? "Observation / Error" : "Observation") : "System"))
                                        property string compactToolText: toolCardMessage
                                            ? ("工具 · " + toolCardName
                                                + " · "
                                                + (toolCardKind === "Action"
                                                    ? (toolCardActionStatus.length > 0 ? toolCardActionStatus : "RUNNING")
                                                    : (toolCardStatus.length > 0 ? toolCardStatus : "DONE")))
                                            : (thinkingMessage
                                                ? rawText.replace("Thinking:", "思考")
                                                : (actionMessage
                                                    ? rawText.replace("Action:", "工具")
                                                    : (observationMessage
                                                        ? rawText.replace("Observation(OK):", "结果:").replace("Observation(ERR):", "错误:")
                                                        : rawText)))
                                        property real measuredTextWidth: bubbleWidthMetrics.advanceWidth
                                        property real stableBubbleWidth: {
                                            if (toolCardMessage) {
                                                return Math.max(260, Math.min(chatList.width * 0.72, 760))
                                            }
                                            var measured = measuredTextWidth + (userMessage ? 30 : 36)
                                            var estimated = root.estimatedChatBubbleWidth(metricText, userMessage, hasLineBreaks, chatList.width)
                                            var base = Math.max(estimated, measured)
                                            var minBubble = userMessage ? 90 : 130
                                            var maxBubble = userMessage
                                                ? Math.min(chatList.width * 0.42, 380)
                                                : Math.min(chatList.width * 0.54, 520)
                                            return Math.min(Math.max(minBubble, base), maxBubble)
                                        }
                                        width: chatList.width
                                        height: compactToolMessage
                                            ? compactToolRow.implicitHeight + 8
                                            : bubbleWrap.height
                                            + ((systemMessage || toolCardMessage) ? 28 : 8)
                                            + (metaInfo.visible ? (metaInfo.implicitHeight + 6) : 0)

                                        TextMetrics {
                                            id: bubbleWidthMetrics
                                            font.pixelSize: root.settingChatFontSize
                                            text: metricText.length > 80 ? metricText.slice(0, 80) : metricText
                                        }

                                        Text {
                                            visible: systemMessage && !compactToolMessage
                                            anchors.left: bubbleWrap.left
                                            anchors.bottom: bubbleWrap.top
                                            anchors.bottomMargin: 5
                                            text: toolCardMessage
                                                ? (preExecCard ? "Tool Pre-Execution" : (toolCardKind === "Action" ? "Tool Action" : "Tool Observation"))
                                                : segmentTitle
                                            color: thinkingMessage ? "#6B7280"
                                                : ((actionMessage || (toolCardMessage && toolCardKind === "Action")) ? "#1D4ED8"
                                                    : (preExecCard ? "#7C3AED"
                                                    : ((observationMessage || (toolCardMessage && toolCardKind === "Observation"))
                                                        ? ((observationError || toolCardStatus === "ERR") ? "#B91C1C" : "#047857")
                                                        : root.textSecondary)))
                                            font.pixelSize: 10
                                            font.bold: true
                                        }

                                        Item {
                                            id: bubbleWrap
                                            visible: !compactToolMessage
                                            width: stableBubbleWidth
                                            height: bubble.height
                                            x: userMessage ? parent.width - width : 0

                                            Rectangle {
                                                id: bubble
                                                width: parent.width
                                                height: toolCardMessage ? toolCardColumn.implicitHeight + 16 : bubbleText.implicitHeight + 22
                                            radius: 18
                                            color: userMessage ? Qt.rgba(210/255, 235/255, 255/255, 0.52)
                                                : (thinkingMessage ? Qt.rgba(235/255, 245/255, 255/255, 0.50)
                                                    : ((actionMessage || (toolCardMessage && toolCardKind === "Action")) ? Qt.rgba(220/255, 240/255, 255/255, 0.50)
                                                        : (preExecCard ? Qt.rgba(225/255, 238/255, 255/255, 0.50)
                                                        : ((observationMessage || (toolCardMessage && toolCardKind === "Observation"))
                                                            ? ((observationError || toolCardStatus === "ERR") ? Qt.rgba(255/255, 230/255, 230/255, 0.50) : Qt.rgba(220/255, 245/255, 235/255, 0.48))
                                                            : Qt.rgba(235/255, 245/255, 255/255, 0.48)))))
                                            border.color: userMessage ? Qt.rgba(150/255, 210/255, 255/255, 0.45)
                                                : (thinkingMessage ? Qt.rgba(175/255, 210/255, 248/255, 0.36)
                                                    : ((actionMessage || (toolCardMessage && toolCardKind === "Action")) ? Qt.rgba(140/255, 200/255, 255/255, 0.36)
                                                        : (preExecCard ? Qt.rgba(165/255, 155/255, 235/255, 0.36)
                                                        : ((observationMessage || (toolCardMessage && toolCardKind === "Observation"))
                                                            ? ((observationError || toolCardStatus === "ERR") ? Qt.rgba(235/255, 140/255, 140/255, 0.40) : Qt.rgba(130/255, 210/255, 175/255, 0.36))
                                                            : Qt.rgba(180/255, 215/255, 250/255, 0.32)))))
                                            border.width: 0.5

                                            Rectangle {
                                                anchors {
                                                    top: parent.top
                                                    horizontalCenter: parent.horizontalCenter
                                                }
                                                width: parent.width * 0.72
                                                height: 0.5
                                                radius: 0.25
                                                color: userMessage ? Qt.rgba(1, 1, 1, 0.60) : Qt.rgba(1, 1, 1, 0.52)
                                            }

                                            Rectangle {
                                                anchors {
                                                    left: parent.left
                                                    leftMargin: 1
                                                    top: parent.top
                                                    topMargin: 1
                                                    right: parent.right
                                                    rightMargin: 1
                                                    bottom: parent.bottom
                                                    bottomMargin: 1
                                                }
                                                radius: 17
                                                color: "transparent"
                                                border.color: Qt.rgba(1, 1, 1, 0.30)
                                                border.width: 0.5
                                            }

                                            Column {
                                                id: toolCardColumn
                                                visible: toolCardMessage
                                                anchors.fill: parent
                                                anchors.margins: 10
                                                spacing: 8

                                                Row {
                                                    spacing: 8
                                                    Text {
                                                        text: preExecCard ? "⏳" : (toolCardStatus === "ERR" ? "✖" : (toolCardStatus === "OK" || toolCardActionStatus === "DONE" ? "✔" : "●"))
                                                        font.pixelSize: 13
                                                        color: toolCardStatus === "ERR" ? "#B91C1C" : (toolCardStatus === "OK" || toolCardActionStatus === "DONE" ? "#047857" : "#2563EB")
                                                    }
                                                    Text {
                                                        text: toolCardName
                                                        font.bold: true
                                                        color: root.textPrimary
                                                        font.pixelSize: 12
                                                    }
                                                    Text {
                                                        text: preExecCard
                                                            ? toolCardStatus
                                                            : (toolCardKind === "Observation"
                                                                ? toolCardStatus
                                                                : (toolCardActionStatus.length > 0 ? toolCardActionStatus : "RUNNING"))
                                                        color: preExecCard ? "#7C3AED" : (toolCardStatus === "ERR" ? "#B91C1C" : "#047857")
                                                        font.pixelSize: 11
                                                    }
                                                    Item { width: 6; height: 1 }
                                                    Text {
                                                        visible: workflowLikeCard
                                                        text: toolCardExpanded ? "收起" : "展开"
                                                        color: root.textSecondary
                                                        font.pixelSize: 10
                                                    }
                                                }

                                                Rectangle {
                                                    visible: workflowLikeCard
                                                    width: parent.width
                                                    height: 2
                                                    color: Qt.rgba(37 / 255, 99 / 255, 235 / 255, 0.18)
                                                }

                                                Rectangle {
                                                    visible: workflowLikeCard
                                                    width: parent.width
                                                    implicitHeight: 6
                                                    radius: 3
                                                    color: Qt.rgba(148 / 255, 163 / 255, 184 / 255, 0.2)
                                                    border.color: Qt.rgba(148 / 255, 163 / 255, 184 / 255, 0.35)
                                                    property var progressMatch: toolCardBody.match(/进度[:：]\s*(\d+)\s*\/\s*(\d+)/)
                                                    property int doneCount: progressMatch ? Number(progressMatch[1]) : (toolCardActionStatus === "DONE" ? 1 : 0)
                                                    property int totalCount: progressMatch ? Math.max(1, Number(progressMatch[2])) : (workflowLikeCard && toolCardName.indexOf("subagent_") === 0 ? 1 : 2)
                                                    Rectangle {
                                                        anchors.left: parent.left
                                                        anchors.top: parent.top
                                                        anchors.bottom: parent.bottom
                                                        width: parent.width * Math.max(0, Math.min(1, parent.doneCount / parent.totalCount))
                                                        radius: parent.radius
                                                        color: toolCardStatus === "ERR" ? "#DC2626" : "#2563EB"
                                                    }
                                                }

                                                Column {
                                                    visible: toolCardBody.length > 0 && (!workflowLikeCard || toolCardExpanded)
                                                    width: parent.width
                                                    spacing: 6
                                                    Row {
                                                        visible: workflowLikeCard
                                                        width: parent.width
                                                        spacing: 8
                                                        Rectangle {
                                                            width: 2
                                                            height: 24
                                                            color: Qt.rgba(37 / 255, 99 / 255, 235 / 255, 0.28)
                                                        }
                                                        Text {
                                                            width: parent.width - 18
                                                            text: toolCardBody
                                                            color: root.textPrimary
                                                            font.pixelSize: 11
                                                            wrapMode: Text.Wrap
                                                            lineHeightMode: Text.ProportionalHeight
                                                            lineHeight: 1.35
                                                            textFormat: Text.PlainText
                                                        }
                                                    }
                                                    Text {
                                                        visible: !workflowLikeCard
                                                        width: parent.width
                                                        text: toolCardBody
                                                        color: root.textPrimary
                                                        font.pixelSize: 11
                                                        wrapMode: Text.Wrap
                                                        lineHeightMode: Text.ProportionalHeight
                                                        lineHeight: 1.35
                                                        textFormat: Text.PlainText
                                                    }
                                                }

                                                TapHandler {
                                                    enabled: workflowLikeCard
                                                    onTapped: toolCardExpanded = !toolCardExpanded
                                                }
                                            }

                                            TextEdit {
                                                id: bubbleText
                                                visible: !toolCardMessage
                                                anchors.left: parent.left
                                                anchors.top: parent.top
                                                anchors.leftMargin: 11
                                                anchors.topMargin: 11
                                                width: parent.width - 22
                                                text: rawText
                                                textFormat: userMessage ? TextEdit.PlainText : TextEdit.MarkdownText
                                                color: userMessage ? "#0A3D6E" : root.textPrimary
                                                wrapMode: TextEdit.WordWrap
                                                font.pixelSize: root.settingChatFontSize
                                                font.italic: thinkingMessage
                                                readOnly: true
                                                selectByMouse: true
                                                persistentSelection: true
                                                activeFocusOnPress: true
                                            }
                                            GlassMenu {
                                                id: messageMenu

                                                MenuItem {
                                                    text: "复制消息"
                                                    onTriggered: {
                                                        clipboardProxy.text = toolCardMessage ? toolCardBody : rawText
                                                        clipboardProxy.selectAll()
                                                        clipboardProxy.copy()
                                                        clipboardProxy.deselect()
                                                    }
                                                }

                                                MenuItem {
                                                    text: "删除此消息"
                                                    onTriggered: {
                                                        if (backend) {
                                                            backend.deleteMessageAt(index)
                                                        }
                                                    }
                                                }
                                            }

                                            TapHandler {
                                                acceptedButtons: Qt.RightButton
                                                onTapped: function(point) {
                                                    bubble.forceActiveFocus()
                                                    messageMenu.popup()
                                                }
                                            }
                                            }
                                        }

                                        Text {
                                            id: metaInfo
                                            visible: !compactToolMessage && (senderLabel.length > 0 || messageTime.length > 0)
                                            anchors.top: bubbleWrap.bottom
                                            anchors.topMargin: 4
                                            anchors.left: userMessage ? undefined : bubbleWrap.left
                                            anchors.right: userMessage ? bubbleWrap.right : undefined
                                            text: messageTime.length > 0 ? (senderLabel + " · " + messageTime) : senderLabel
                                            color: root.textSecondary
                                            font.pixelSize: 10
                                            horizontalAlignment: userMessage ? Text.AlignRight : Text.AlignLeft
                                        }

                                        Text {
                                            id: compactToolRow
                                            visible: compactToolMessage
                                            anchors.left: parent.left
                                            anchors.leftMargin: 6
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: parent.width - 12
                                            text: compactToolText
                                            color: root.textSecondary
                                            font.pixelSize: 10
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                GlassButton {
                                    text: "滑到底部"
                                    visible: root.activePanel === "chat" && !chatList.atYEnd
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    anchors.rightMargin: 16
                                    anchors.bottomMargin: 16
                                    z: 3
                                    onClicked: {
                                        root.chatAutoScrollEnabled = true
                                        root.chatScrollPending = false
                                        chatScrollTimer.stop()
                                        chatList.positionViewAtEnd()
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 122
                                radius: 16
                                color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.28)
                                border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.22)

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 6

                                    TextArea {
                                        id: inputField
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        placeholderText: "输入消息..."
                                        wrapMode: TextArea.Wrap
                                        readOnly: false
                                        background: null

                                        Keys.onPressed: function(event) {
                                            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                                if ((event.modifiers & Qt.ControlModifier) !== 0) {
                                                    inputField.insert("\n")
                                                } else {
                                                    root.sendCurrentInput()
                                                    event.accepted = true
                                                }
                                            }
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true

                                        GlassButton {
                                            text: "加载文件"
                                            onClicked: {
                                                quickActionDialog.text = "请在左侧会话区使用“导入会话”，或在设置面板加载工作区文件。"
                                                quickActionDialog.open()
                                            }
                                        }

                                        GlassButton {
                                            text: "清空上下文"
                                            onClicked: {
                                                if (backend && backend.clearCurrentContext) {
                                                    backend.clearCurrentContext()
                                                    quickActionDialog.text = "当前会话上下文已清空。"
                                                    quickActionDialog.open()
                                                }
                                            }
                                        }

                                        GlassButton {
                                            text: "压缩上下文"
                                            onClicked: {
                                                if (backend && backend.compressCurrentContext) {
                                                    var compressResult = backend.compressCurrentContext()
                                                    root.loadCompressionInfo()
                                                    quickActionDialog.text = compressResult
                                                    quickActionDialog.open()
                                                }
                                            }
                                        }

                                        GlassButton {
                                            text: "打开记忆"
                                            onClicked: {
                                                root.openPanel("memory")
                                                quickActionDialog.text = "已切换到 Memory 面板，可查看/编辑 MEMORY 与 HISTORY。"
                                                quickActionDialog.open()
                                            }
                                        }

                                        GlassButton {
                                            text: "导出会话"
                                            onClicked: {
                                                if (backend && backend.exportCurrentSession) {
                                                    var exportResult = backend.exportCurrentSession()
                                                    quickActionDialog.text = exportResult
                                                    quickActionDialog.open()
                                                }
                                            }
                                        }

                                        GlassButton {
                                            text: "工作流模板"
                                            onClicked: {
                                                workflowDialog.templateId = "research"
                                                workflowDialog.modeId = "sequential"
                                                workflowTemplateSelector.currentIndex = 0
                                                workflowModeSelector.currentIndex = 0
                                                workflowDialog.open()
                                            }
                                        }

                                        GlassButton {
                                            text: "快速子代理"
                                            onClicked: {
                                                if (backend && backend.runQuickSubagentTask) {
                                                    var subResult = backend.runQuickSubagentTask("请用 5 行总结当前会话的下一步实施建议。")
                                                    quickActionDialog.text = subResult
                                                    quickActionDialog.open()
                                                } else {
                                                    quickActionDialog.text = "Quick Subagent 不可用"
                                                    quickActionDialog.open()
                                                }
                                            }
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }

                                        Text {
                                            text: "回车发送，Ctrl+回车换行"
                                            color: root.textSecondary
                                            font.pixelSize: 11
                                        }

                                        GlassButton {
                                            text: "滑到底部"
                                            visible: chatList && !chatList.atYEnd
                                            onClicked: {
                                                root.chatAutoScrollEnabled = true
                                                root.chatScrollPending = false
                                                chatScrollTimer.stop()
                                                chatList.positionViewAtEnd()
                                            }
                                        }

                                        GlassButton {
                                            text: "发送"
                                            enabled: !(backend ? backend.generating : false)
                                            onClicked: root.sendCurrentInput()
                                        }

                                        GlassButton {
                                            text: "停止"
                                            visible: true
                                            enabled: backend ? backend.generating : false
                                            onClicked: {
                                                if (backend) {
                                                    backend.stopGeneration()
                                                }
                                            }
                                        }

                                        GlassButton {
                                            text: "测试 API"
                                            enabled: true
                                            onClicked: {
                                                if (backend) {
                                                    var probeResult = backend.quickApiProbe ? backend.quickApiProbe() : "API 测试不可用"
                                                    if (probeResultDialog) {
                                                        probeResultDialog.text = probeResult
                                                        probeResultDialog.open()
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 16
                        color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.20)
                        border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    text: "设置"
                                    color: root.textSecondary
                                    font.pixelSize: 11
                                }

                                Item { Layout.fillWidth: true }

                                GlassButton {
                                    text: "重载"
                                    onClicked: root.loadSettingsContent()
                                }

                                GlassButton {
                                    text: "测试 API"
                                    onClicked: root.runSettingsTest()
                                }

                                GlassButton {
                                    text: "测飞书/Telegram"
                                    onClicked: root.runExternalTest()
                                }

                                GlassButton {
                                    text: "保存"
                                    onClicked: root.saveSettingsContent()
                                }
                            }

                            ScrollView {
                                id: settingsScroll
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                background: null
                                clip: true

                                ColumnLayout {
                                    width: settingsScroll.availableWidth
                                    spacing: 10

                                    Text {
                                        text: "LLM 与平台配置"
                                        color: root.accent
                                        font.pixelSize: 11
                                        font.bold: true
                                        font.capitalization: Font.AllUppercase
                                        font.letterSpacing: 1.1
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Text {
                                            text: "聊天字体"
                                            color: root.textPrimary
                                            font.pixelSize: 11
                                            Layout.preferredWidth: 72
                                        }

                                        Slider {
                                            id: chatFontSizeSlider
                                            Layout.fillWidth: true
                                            from: 11
                                            to: 20
                                            stepSize: 1
                                            value: root.settingChatFontSize
                                            onMoved: root.settingChatFontSize = Math.round(value)
                                        }

                                        Text {
                                            text: root.settingChatFontSize + " px"
                                            color: root.textSecondary
                                            font.pixelSize: 11
                                            Layout.preferredWidth: 52
                                            horizontalAlignment: Text.AlignRight
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Text {
                                            text: "聊天行距"
                                            color: root.textPrimary
                                            font.pixelSize: 11
                                            Layout.preferredWidth: 72
                                        }

                                        Slider {
                                            id: chatLineHeightSlider
                                            Layout.fillWidth: true
                                            from: 1.15
                                            to: 2.0
                                            stepSize: 0.05
                                            value: root.settingChatLineHeight
                                            onMoved: root.settingChatLineHeight = Math.round(value * 100) / 100
                                        }

                                        Text {
                                            text: root.settingChatLineHeight.toFixed(2)
                                            color: root.textSecondary
                                            font.pixelSize: 11
                                            Layout.preferredWidth: 52
                                            horizontalAlignment: Text.AlignRight
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 1
                                        color: Qt.rgba(108 / 255, 124 / 255, 152 / 255, 0.2)
                                    }

                                    Text {
                                        text: "界面背景与玻璃"
                                        color: root.accent
                                        font.pixelSize: 11
                                        font.bold: true
                                        font.capitalization: Font.AllUppercase
                                        font.letterSpacing: 1.1
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        GlassTextField {
                                            Layout.fillWidth: true
                                            text: root.settingBackgroundImagePath
                                            placeholderText: "背景图片本地路径"
                                            onTextChanged: root.settingBackgroundImagePath = text
                                        }

                                        GlassButton {
                                            text: "选择图片"
                                            onClicked: backgroundImageDialog.open()
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Text {
                                            text: "背景模糊"
                                            color: root.textPrimary
                                            font.pixelSize: 11
                                            Layout.preferredWidth: 72
                                        }

                                        Slider {
                                            id: backgroundBlurSlider
                                            Layout.fillWidth: true
                                            from: 0
                                            to: 36
                                            stepSize: 1
                                            value: root.settingBackgroundBlurRadius
                                            onMoved: {
                                                root.settingBackgroundBlurRadius = Math.round(value)
                                                settingsSaveTimer.restart()
                                            }
                                        }

                                        Text {
                                            text: root.settingBackgroundBlurRadius <= 0 ? "清晰" : ("模糊 " + Math.round(root.settingBackgroundBlurRadius))
                                            color: root.textSecondary
                                            font.pixelSize: 11
                                            Layout.preferredWidth: 64
                                            horizontalAlignment: Text.AlignRight
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Text {
                                            text: "面板清晰度"
                                            color: root.textPrimary
                                            font.pixelSize: 11
                                            Layout.preferredWidth: 72
                                        }

                                        Slider {
                                            id: panelOpacitySlider
                                            Layout.fillWidth: true
                                            from: 0.35
                                            to: 0.95
                                            stepSize: 0.01
                                            value: root.settingPanelOpacity
                                            onMoved: root.settingPanelOpacity = Math.round(value * 100) / 100
                                        }

                                        Text {
                                            text: Math.round(root.settingPanelOpacity * 100) + "%"
                                            color: root.textSecondary
                                            font.pixelSize: 11
                                            Layout.preferredWidth: 64
                                            horizontalAlignment: Text.AlignRight
                                        }
                                    }

                                    GlassTextField {
                                        id: apiKeyField
                                        Layout.fillWidth: true
                                        placeholderText: "API Key"
                                        text: root.settingApiKey
                                        echoMode: TextInput.Password
                                        onTextChanged: root.settingApiKey = text
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                    GlassComboBox {
                                        id: providerUrlSelector
                                        Layout.fillWidth: true
                                        implicitHeight: 34
                                        model: providerPresetModel
                                        textRole: "label"
                                        displayText: root.currentProviderPresetLabel()
                                        background: Rectangle {
                                            radius: 10
                                            color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.26)
                                            border.color: Qt.rgba(120 / 255, 138 / 255, 168 / 255, 0.32)
                                        }
                                        delegate: ItemDelegate {
                                            width: providerUrlSelector.width
                                            implicitHeight: 30
                                            contentItem: Text {
                                                text: (typeof label !== "undefined" && label) ? label : ""
                                                color: root.textPrimary
                                                elide: Text.ElideRight
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                        }
                                        onActivated: {
                                            root.applyProviderPreset(currentIndex, true)
                                        }
                                        onCurrentIndexChanged: {
                                            if (currentIndex >= 0 && currentIndex < providerPresetModel.count) {
                                                var selected = providerPresetModel.get(currentIndex)
                                                if (selected && selected.type && selected.type !== root.settingProviderType) {
                                                    root.applyProviderPreset(currentIndex, true)
                                                }
                                            }
                                        }
                                    }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: "当前 Provider: " + root.currentProviderPresetLabel()
                                        color: root.textPrimary
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        text: "Provider 类型: " + root.settingProviderType + "（URL 使用预置项）"
                                        color: root.textSecondary
                                        font.pixelSize: 10
                                    }

                                    GlassTextField {
                                        id: baseUrlField
                                        Layout.fillWidth: true
                                        placeholderText: "Base URL（只读）"
                                        text: root.settingBaseUrl
                                        readOnly: true
                                    }

                                    GlassComboBox {
                                        id: providerModelSelector
                                        Layout.fillWidth: true
                                        implicitHeight: 34
                                        model: providerModelOptions
                                        textRole: "name"
                                        displayText: root.currentProviderModelLabel()
                                        background: Rectangle {
                                            radius: 10
                                            color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.26)
                                            border.color: Qt.rgba(120 / 255, 138 / 255, 168 / 255, 0.32)
                                        }
                                        delegate: ItemDelegate {
                                            width: providerModelSelector.width
                                            implicitHeight: 30
                                            contentItem: Text {
                                                text: (typeof name !== "undefined" && name) ? name : ""
                                                color: root.textPrimary
                                                elide: Text.ElideRight
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                        }
                                        onActivated: {
                                            var row = providerModelOptions.get(currentIndex)
                                            if (row && row.name) {
                                                root.settingModel = row.name
                                                if (modelField) {
                                                    modelField.text = row.name
                                                }
                                            }
                                        }
                                        onCurrentIndexChanged: {
                                            if (currentIndex >= 0 && currentIndex < providerModelOptions.count) {
                                                var selectedModel = providerModelOptions.get(currentIndex)
                                                if (selectedModel && selectedModel.name && root.settingModel !== selectedModel.name) {
                                                    root.settingModel = selectedModel.name
                                                    if (modelField) {
                                                        modelField.text = selectedModel.name
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: "当前模型: " + root.currentProviderModelLabel()
                                        color: root.textPrimary
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }

                                    GlassTextField {
                                        id: modelField
                                        Layout.fillWidth: true
                                        placeholderText: "模型名称（可手动修改）"
                                        text: root.settingModel
                                        onTextChanged: root.settingModel = text
                                    }

                                    GlassTextField {
                                        id: telegramField
                                        Layout.fillWidth: true
                                        visible: root.isChannelSelected("telegram")
                                        placeholderText: "Telegram Token"
                                        text: root.settingTelegramToken
                                        echoMode: TextInput.Password
                                        onTextChanged: root.settingTelegramToken = text
                                    }

                                    GlassTextField {
                                        id: feishuAppIdField
                                        Layout.fillWidth: true
                                        visible: root.isChannelSelected("feishu")
                                        placeholderText: "Feishu App ID"
                                        text: root.settingFeishuAppId
                                        onTextChanged: root.settingFeishuAppId = text
                                    }

                                    GlassTextField {
                                        id: feishuSecretField
                                        Layout.fillWidth: true
                                        visible: root.isChannelSelected("feishu")
                                        placeholderText: "Feishu App Secret"
                                        text: root.settingFeishuAppSecret
                                        echoMode: TextInput.Password
                                        onTextChanged: root.settingFeishuAppSecret = text
                                    }

                                    GlassTextField {
                                        id: feishuVerifyField
                                        Layout.fillWidth: true
                                        visible: root.isChannelSelected("feishu")
                                        placeholderText: "Feishu Verification Token"
                                        text: root.settingFeishuVerificationToken
                                        echoMode: TextInput.Password
                                        onTextChanged: root.settingFeishuVerificationToken = text
                                    }

                                    SpinBox {
                                        id: feishuPortField
                                        Layout.fillWidth: true
                                        visible: root.isChannelSelected("feishu")
                                        from: 1
                                        to: 65535
                                        value: root.settingFeishuPort
                                        onValueModified: root.settingFeishuPort = value
                                    }

                                    GlassComboBox {
                                        id: messageChannelSelector
                                        Layout.fillWidth: true
                                        model: ["telegram", "feishu", "discord", "dingtalk", "wechat", "qq", "wecom"]
                                        Component.onCompleted: {
                                            var idx = model.indexOf(root.settingMessageChannel)
                                            currentIndex = idx >= 0 ? idx : 0
                                        }
                                        onActivated: {
                                            root.settingMessageChannel = model[currentIndex]
                                        }
                                    }

                                    GlassTextField {
                                        id: discordWebhookField
                                        Layout.fillWidth: true
                                        visible: root.isChannelSelected("discord")
                                        placeholderText: "Discord Webhook URL"
                                        text: root.settingDiscordWebhookUrl
                                        onTextChanged: root.settingDiscordWebhookUrl = text
                                    }

                                    GlassTextField {
                                        id: dingtalkWebhookField
                                        Layout.fillWidth: true
                                        visible: root.isChannelSelected("dingtalk")
                                        placeholderText: "DingTalk Webhook URL"
                                        text: root.settingDingTalkWebhookUrl
                                        onTextChanged: root.settingDingTalkWebhookUrl = text
                                    }

                                    GlassTextField {
                                        id: wechatWebhookField
                                        Layout.fillWidth: true
                                        visible: root.isChannelSelected("wechat")
                                        placeholderText: "Wechat Webhook URL"
                                        text: root.settingWechatWebhookUrl
                                        onTextChanged: root.settingWechatWebhookUrl = text
                                    }

                                    GlassTextField {
                                        id: qqWebhookField
                                        Layout.fillWidth: true
                                        visible: root.isChannelSelected("qq")
                                        placeholderText: "QQ Bot Webhook URL"
                                        text: root.settingQqWebhookUrl
                                        onTextChanged: root.settingQqWebhookUrl = text
                                    }

                                    GlassTextField {
                                        id: wecomWebhookField
                                        Layout.fillWidth: true
                                        visible: root.isChannelSelected("wecom")
                                        placeholderText: "Wecom Webhook URL"
                                        text: root.settingWecomWebhookUrl
                                        onTextChanged: root.settingWecomWebhookUrl = text
                                    }

                                    Text {
                                        visible: root.isChannelSelected("feishu")
                                        text: "飞书端口（默认 8080）用于本地接收飞书回调，不是 AI 模型配置。"
                                        color: root.textSecondary
                                        font.pixelSize: 10
                                        wrapMode: Text.WordWrap
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 36
                                        radius: 10
                                        color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.09)
                                        border.color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.24)

                                        Text {
                                            anchors.fill: parent
                                            anchors.leftMargin: 10
                                            anchors.rightMargin: 10
                                            verticalAlignment: Text.AlignVCenter
                                            text: root.settingsTestResult.length > 0 ? root.settingsTestResult : "填写配置后可测试并保存。"
                                            color: root.textSecondary
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 36
                                        radius: 10
                                        color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.09)
                                        border.color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.24)

                                        Text {
                                            anchors.fill: parent
                                            anchors.leftMargin: 10
                                            anchors.rightMargin: 10
                                            verticalAlignment: Text.AlignVCenter
                                            text: root.settingsExternalTestResult.length > 0 ? root.settingsExternalTestResult : "可测试 Telegram 与飞书凭据。"
                                            color: root.textSecondary
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 1
                                        color: Qt.rgba(108 / 255, 124 / 255, 152 / 255, 0.28)
                                    }

                                    Text {
                                        text: "Agent 文档入口"
                                        color: root.accent
                                        font.pixelSize: 11
                                        font.bold: true
                                        font.capitalization: Font.AllUppercase
                                        font.letterSpacing: 1.1
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        GlassComboBox {
                                            id: agentDocSelector
                                            Layout.fillWidth: true
                                            model: agentDocEntries
                                            textRole: "title"
                                            background: Rectangle {
                                                radius: 10
                                                color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.26)
                                                border.color: Qt.rgba(120 / 255, 138 / 255, 168 / 255, 0.32)
                                            }
                                            onActivated: {
                                                var row = agentDocEntries.get(currentIndex)
                                                if (row && row.path) {
                                                    root.loadAgentDocContent(row.path)
                                                }
                                            }
                                            Component.onCompleted: {
                                                var matched = -1
                                                for (var i = 0; i < agentDocEntries.count; ++i) {
                                                    if (agentDocEntries.get(i).path === root.agentDocPath) {
                                                        matched = i
                                                        break
                                                    }
                                                }
                                                if (matched >= 0) {
                                                    currentIndex = matched
                                                }
                                            }
                                        }

                                        GlassButton {
                                            text: "加载"
                                            onClicked: {
                                                var row = agentDocEntries.get(agentDocSelector.currentIndex)
                                                if (row && row.path) {
                                                    root.loadAgentDocContent(row.path)
                                                }
                                            }
                                        }

                                        GlassButton {
                                            text: "保存文档"
                                            onClicked: root.saveAgentDocContent()
                                        }
                                    }

                                    GlassTextField {
                                        Layout.fillWidth: true
                                        readOnly: true
                                        text: root.agentDocPath
                                        placeholderText: "文档路径"
                                    }

                                    TextArea {
                                        id: agentDocEditor
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 260
                                        text: root.agentDocContent
                                        wrapMode: TextArea.Wrap
                                        color: root.textPrimary
                                        selectByMouse: true
                                        onTextChanged: root.agentDocContent = text
                                        background: Rectangle {
                                            radius: 12
                                            color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                            border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 36
                                        radius: 10
                                        color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.09)
                                        border.color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.24)

                                        Text {
                                            anchors.fill: parent
                                            anchors.leftMargin: 10
                                            anchors.rightMargin: 10
                                            verticalAlignment: Text.AlignVCenter
                                            text: root.agentDocStatus.length > 0 ? root.agentDocStatus : "可直接编辑 SOUL/AGENTS/USER/TOOLS 等文档。"
                                            color: root.textSecondary
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 16
                        color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.20)
                        border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    text: "日志"
                                    color: root.textSecondary
                                    font.pixelSize: 11
                                }

                                Item { Layout.fillWidth: true }

                                GlassButton {
                                    text: "刷新"
                                    onClicked: root.loadLogsContent()
                                }
                            }

                            ScrollView {
                                background: null
                                clip: true
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                TextArea {
                                    width: parent.width
                                    text: root.logsContent
                                    readOnly: true
                                    wrapMode: TextArea.Wrap
                                    color: root.textPrimary
                                    background: Rectangle {
                                        radius: 12
                                        color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                        border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 16
                        color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.20)
                        border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    text: "Memory"
                                    color: root.textSecondary
                                    font.pixelSize: 11
                                }

                                Item { Layout.fillWidth: true }

                                GlassButton {
                                    text: "刷新"
                                    onClicked: root.loadMemoryContent()
                                }

                                GlassButton {
                                    text: "保存"
                                    onClicked: root.saveMemoryContent()
                                }

                                GlassButton {
                                    text: "清空"
                                    onClicked: {
                                        if (backend && backend.clearMemoryStorage) {
                                            backend.clearMemoryStorage()
                                            root.loadMemoryContent()
                                            root.memoryOverviewStatus = "memory 已清空"
                                        } else {
                                            root.memoryContent = ""
                                            root.historyContent = ""
                                            root.saveMemoryContent()
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 30
                                radius: 8
                                color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.09)
                                border.color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.24)

                                Text {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    verticalAlignment: Text.AlignVCenter
                                    text: root.memoryOverviewStatus.length > 0 ? root.memoryOverviewStatus : "可编辑 MEMORY.md / HISTORY.md，支持检索与手动追加记忆。"
                                    color: root.textSecondary
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 8

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    TextArea {
                                        width: parent.width
                                        text: root.memoryContent
                                        wrapMode: TextArea.Wrap
                                        color: root.textPrimary
                                        selectByMouse: true
                                        onTextChanged: root.memoryContent = text
                                        background: Rectangle {
                                            radius: 12
                                            color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                            border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)
                                        }
                                    }
                                }

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    TextArea {
                                        width: parent.width
                                        text: root.historyContent
                                        wrapMode: TextArea.Wrap
                                        color: root.textPrimary
                                        selectByMouse: true
                                        onTextChanged: root.historyContent = text
                                        background: Rectangle {
                                            radius: 12
                                            color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                            border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                GlassTextField {
                                    id: memoryQueryField
                                    Layout.fillWidth: true
                                    placeholderText: "检索记忆（关键词）"
                                    text: root.memorySearchQuery
                                    onTextChanged: root.memorySearchQuery = text
                                }

                                GlassButton {
                                    text: "检索"
                                    onClicked: root.runMemorySearch()
                                }

                                GlassButton {
                                    text: "追加记忆"
                                    onClicked: {
                                        root.appendManualMemoryNote(root.memorySearchQuery)
                                    }
                                }
                            }

                            ListView {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 110
                                clip: true
                                spacing: 4
                                model: memorySearchModel

                                delegate: Rectangle {
                                    required property var model
                                    width: ListView.view.width
                                    implicitHeight: 34
                                    radius: 8
                                    color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                    border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                    Text {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        verticalAlignment: Text.AlignVCenter
                                        text: model.text ? model.text : ""
                                        color: root.textSecondary
                                        font.pixelSize: 10
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 16
                        color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.20)
                        border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    text: "Token 统计"
                                    color: root.textSecondary
                                    font.pixelSize: 11
                                }

                                Text {
                                    text: "年份"
                                    color: root.textSecondary
                                    font.pixelSize: 10
                                }

                                GlassComboBox {
                                    id: tokenYearSelector
                                    model: tokenYearOptionsModel
                                    textRole: "label"
                                    implicitWidth: 96
                                    onActivated: {
                                        var selectedYear = tokenYearOptionsModel.get(currentIndex)
                                        if (selectedYear && selectedYear.year) {
                                            root.tokenSelectedYear = Number(selectedYear.year)
                                        }
                                    }
                                }

                                Text {
                                    text: "窗口"
                                    color: root.textSecondary
                                    font.pixelSize: 10
                                }

                                GlassComboBox {
                                    id: tokenDaysSelector
                                    model: [7, 30, 90, 180, 365]
                                    implicitWidth: 90
                                    onActivated: {
                                        root.tokenChartDays = Number(model[currentIndex])
                                    }
                                    Component.onCompleted: {
                                        var idx = model.indexOf(root.tokenChartDays)
                                        currentIndex = idx >= 0 ? idx : 1
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                Text {
                                    text: root.tokenAnalyticsStatus
                                    color: root.textSecondary
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                    Layout.preferredWidth: 260
                                    horizontalAlignment: Text.AlignRight
                                }

                                GlassButton {
                                    text: "刷新"
                                    onClicked: root.loadTokenStats(true)
                                }
                            }

                            ScrollView {
                                id: tokenScroll
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                background: null
                                clip: true

                                ColumnLayout {
                                    width: tokenScroll.availableWidth
                                    spacing: 10

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 44
                                        radius: 10
                                        color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.1)
                                        border.color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.24)

                                        Text {
                                            anchors.centerIn: parent
                                            text: root.tokenStatsText.length > 0 ? root.tokenStatsText : "本轮消耗(API)：0 Token"
                                            color: root.accent
                                            font.pixelSize: 12
                                            font.bold: true
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: 72
                                            radius: 12
                                            color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                            border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                            ColumnLayout {
                                                anchors.fill: parent
                                                anchors.margins: 8
                                                spacing: 2

                                                Text { text: "本月总量"; color: root.textSecondary; font.pixelSize: 10 }
                                                Text { text: String(root.tokenCurrentMonthTotal); color: root.textPrimary; font.pixelSize: 16; font.bold: true }
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: 72
                                            radius: 12
                                            color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                            border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                            ColumnLayout {
                                                anchors.fill: parent
                                                anchors.margins: 8
                                                spacing: 2

                                                Text { text: "本年总量"; color: root.textSecondary; font.pixelSize: 10 }
                                                Text { text: String(root.tokenCurrentYearTotal); color: root.textPrimary; font.pixelSize: 16; font.bold: true }
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: 72
                                            radius: 12
                                            color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                            border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                            ColumnLayout {
                                                anchors.fill: parent
                                                anchors.margins: 8
                                                spacing: 2

                                                Text { text: "累计总量"; color: root.textSecondary; font.pixelSize: 10 }
                                                Text { text: String(root.tokenAllTimeTotal); color: root.textPrimary; font.pixelSize: 16; font.bold: true }
                                            }
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: 72
                                            radius: 12
                                            color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                            border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                            ColumnLayout {
                                                anchors.fill: parent
                                                anchors.margins: 8
                                                spacing: 2

                                                Text { text: "本轮 Prompt"; color: root.textSecondary; font.pixelSize: 10 }
                                                Text { text: String(root.tokenLastPrompt); color: root.textPrimary; font.pixelSize: 16; font.bold: true }
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: 72
                                            radius: 12
                                            color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                            border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                            ColumnLayout {
                                                anchors.fill: parent
                                                anchors.margins: 8
                                                spacing: 2

                                                Text { text: "本轮 Completion"; color: root.textSecondary; font.pixelSize: 10 }
                                                Text { text: String(root.tokenLastCompletion); color: root.textPrimary; font.pixelSize: 16; font.bold: true }
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: 72
                                            radius: 12
                                            color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                            border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                            ColumnLayout {
                                                anchors.fill: parent
                                                anchors.margins: 8
                                                spacing: 2

                                                Text { text: "本轮 Total"; color: root.textSecondary; font.pixelSize: 10 }
                                                Text { text: String(root.tokenLastTotal); color: root.textPrimary; font.pixelSize: 16; font.bold: true }
                                            }
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 62
                                        radius: 12
                                        color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                        border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            spacing: 8

                                            CheckBox {
                                                text: "Prompt"
                                                checked: root.tokenShowPrompt
                                                onToggled: {
                                                    root.tokenShowPrompt = checked
                                                    root.tokenDailyMax = root.computeTokenSeriesMax(tokenDailyModel)
                                                    root.tokenMonthlyMax = root.computeTokenSeriesMax(tokenMonthlyModel)
                                                    root.tokenYearlyMax = root.computeTokenSeriesMax(tokenYearlyModel)
                                                }
                                            }

                                            CheckBox {
                                                text: "Completion"
                                                checked: root.tokenShowCompletion
                                                onToggled: {
                                                    root.tokenShowCompletion = checked
                                                    root.tokenDailyMax = root.computeTokenSeriesMax(tokenDailyModel)
                                                    root.tokenMonthlyMax = root.computeTokenSeriesMax(tokenMonthlyModel)
                                                    root.tokenYearlyMax = root.computeTokenSeriesMax(tokenYearlyModel)
                                                }
                                            }

                                            CheckBox {
                                                text: "Total"
                                                checked: root.tokenShowTotal
                                                onToggled: {
                                                    root.tokenShowTotal = checked
                                                    root.tokenDailyMax = root.computeTokenSeriesMax(tokenDailyModel)
                                                    root.tokenMonthlyMax = root.computeTokenSeriesMax(tokenMonthlyModel)
                                                    root.tokenYearlyMax = root.computeTokenSeriesMax(tokenYearlyModel)
                                                }
                                            }

                                            Item { Layout.fillWidth: true }

                                            Text {
                                                text: "缩放视图：" + root.visibleDailyCount() + " 天"
                                                color: root.textSecondary
                                                font.pixelSize: 10
                                            }

                                            GlassButton {
                                                text: "-"
                                                onClicked: root.zoomDaily(0.8)
                                            }

                                            GlassButton {
                                                text: "+"
                                                onClicked: root.zoomDaily(1.25)
                                            }

                                            GlassButton {
                                                text: "重置"
                                                onClicked: root.resetDailyZoom()
                                            }
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 210
                                        radius: 12
                                        color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                        border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            spacing: 6

                                            Text {
                                                text: "最近 " + root.tokenChartDays + " 天每日 Token 用量"
                                                color: root.textSecondary
                                                font.pixelSize: 10
                                            }

                                            Rectangle {
                                                id: tokenDailyPlot
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                radius: 8
                                                color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.04)
                                                border.color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.18)

                                                MouseArea {
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    onWheel: function(wheel) {
                                                        if (wheel.angleDelta.y > 0) {
                                                            root.zoomDaily(0.9)
                                                        } else if (wheel.angleDelta.y < 0) {
                                                            root.zoomDaily(1.1)
                                                        }
                                                    }
                                                }

                                                Row {
                                                    anchors.fill: parent
                                                    anchors.margins: 6
                                                    spacing: 2

                                                    Repeater {
                                                        model: root.visibleDailyCount()

                                                        delegate: Column {
                                                            property int sourceIndex: root.tokenDailyViewStart + index
                                                            property var rowData: tokenDailyModel.get(sourceIndex)
                                                            property string label: rowData && rowData.label ? rowData.label : ""
                                                            property int prompt: rowData && rowData.prompt ? Number(rowData.prompt) : 0
                                                            property int completion: rowData && rowData.completion ? Number(rowData.completion) : 0
                                                            property int total: rowData && rowData.total ? Number(rowData.total) : (prompt + completion)
                                                            width: Math.max(8, (tokenDailyPlot.width - 12 - Math.max(0, root.visibleDailyCount() - 1) * 2) / Math.max(1, root.visibleDailyCount()))
                                                            height: tokenDailyPlot.height - 12
                                                            spacing: 2

                                                            Item {
                                                                width: parent.width
                                                                height: parent.height - 16

                                                                MouseArea {
                                                                    anchors.fill: parent
                                                                    hoverEnabled: true
                                                                    onEntered: root.tokenHoveredDailyIndex = sourceIndex
                                                                    onExited: {
                                                                        if (root.tokenHoveredDailyIndex === sourceIndex) {
                                                                            root.tokenHoveredDailyIndex = -1
                                                                        }
                                                                    }
                                                                }

                                                                Rectangle {
                                                                    anchors.left: parent.left
                                                                    anchors.right: parent.right
                                                                    anchors.bottom: parent.bottom
                                                                    radius: 2
                                                                    height: Math.max(2, ((root.tokenShowTotal ? total : Math.max(prompt, completion)) / Math.max(1, root.tokenDailyMax)) * (parent.height - 2))
                                                                    color: sourceIndex === root.tokenHoveredDailyIndex
                                                                        ? Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.9)
                                                                        : Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.72)
                                                                }
                                                            }

                                                            Text {
                                                                width: parent.width
                                                                text: (root.visibleDailyCount() <= 14 || index % 5 === 0) ? label : ""
                                                                horizontalAlignment: Text.AlignHCenter
                                                                color: root.textSecondary
                                                                font.pixelSize: 8
                                                                elide: Text.ElideRight
                                                            }
                                                        }
                                                    }
                                                }

                                                Rectangle {
                                                    visible: root.tokenHoveredDailyIndex >= 0
                                                    anchors.right: parent.right
                                                    anchors.top: parent.top
                                                    anchors.margins: 8
                                                    radius: 8
                                                    color: Qt.rgba(15/255, 23/255, 42/255, 0.86)
                                                    border.color: Qt.rgba(1, 1, 1, 0.24)
                                                    implicitWidth: 190
                                                    implicitHeight: 74

                                                    Column {
                                                        id: dailyTooltip
                                                        anchors.fill: parent
                                                        anchors.margins: 8
                                                        spacing: 2
                                                        property var hoveredDaily: root.tokenHoveredDailyIndex >= 0 && root.tokenHoveredDailyIndex < tokenDailyModel.count
                                                            ? tokenDailyModel.get(root.tokenHoveredDailyIndex) : null
                                                        Text { text: dailyTooltip.hoveredDaily ? dailyTooltip.hoveredDaily.date : ""; color: "white"; font.pixelSize: 10; font.bold: true }
                                                        Text { text: "P: " + root.formatTokenValue(dailyTooltip.hoveredDaily ? dailyTooltip.hoveredDaily.prompt : 0); color: "#C7D2FE"; font.pixelSize: 10; visible: root.tokenShowPrompt }
                                                        Text { text: "C: " + root.formatTokenValue(dailyTooltip.hoveredDaily ? dailyTooltip.hoveredDaily.completion : 0); color: "#86EFAC"; font.pixelSize: 10; visible: root.tokenShowCompletion }
                                                        Text { text: "T: " + root.formatTokenValue(dailyTooltip.hoveredDaily ? dailyTooltip.hoveredDaily.total : 0); color: "#FDBA74"; font.pixelSize: 10; visible: root.tokenShowTotal }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 210
                                        radius: 12
                                        color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                        border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            spacing: 6

                                            Text {
                                                text: root.tokenSelectedYear + " 年每月 Token 用量"
                                                color: root.textSecondary
                                                font.pixelSize: 10
                                            }

                                            Rectangle {
                                                id: tokenMonthlyPlot
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                radius: 8
                                                color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.04)
                                                border.color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.18)

                                                Column {
                                                    anchors.left: parent.left
                                                    anchors.top: parent.top
                                                    anchors.bottom: parent.bottom
                                                    anchors.leftMargin: 2
                                                    anchors.topMargin: 8
                                                    anchors.bottomMargin: 18
                                                    spacing: 0

                                                    Repeater {
                                                        model: 4
                                                        delegate: Text {
                                                            required property int index
                                                            text: root.formatTokenValue(Math.round(root.tokenMonthlyMax * (1 - index / 3)))
                                                            color: root.textSecondary
                                                            font.pixelSize: 8
                                                        }
                                                    }
                                                }

                                                Row {
                                                    anchors.fill: parent
                                                    anchors.leftMargin: 28
                                                    anchors.rightMargin: 6
                                                    anchors.topMargin: 6
                                                    anchors.bottomMargin: 6
                                                    spacing: 3

                                                    Repeater {
                                                        model: tokenMonthlyModel

                                                        delegate: Column {
                                                            id: monthlyDelegate
                                                            required property int index
                                                            required property int month
                                                            required property int total
                                                            width: Math.max(14, (tokenMonthlyPlot.width - 12 - 11 * 3) / 12)
                                                            height: tokenMonthlyPlot.height - 12
                                                            spacing: 2

                                                            Item {
                                                                width: parent.width
                                                                height: parent.height - 16

                                                                MouseArea {
                                                                    anchors.fill: parent
                                                                    hoverEnabled: true
                                                                    onEntered: root.tokenHoveredMonthlyIndex = monthlyDelegate.index
                                                                    onExited: {
                                                                        if (root.tokenHoveredMonthlyIndex === monthlyDelegate.index) {
                                                                            root.tokenHoveredMonthlyIndex = -1
                                                                        }
                                                                    }
                                                                }

                                                                Rectangle {
                                                                    anchors.left: parent.left
                                                                    anchors.right: parent.right
                                                                    anchors.bottom: parent.bottom
                                                                    radius: 2
                                                                    height: Math.max(2, (total / Math.max(1, root.tokenMonthlyMax)) * (parent.height - 2))
                                                                    color: Qt.rgba(56 / 255, 161 / 255, 105 / 255, 0.72)
                                                                }
                                                            }

                                                            Text {
                                                                width: parent.width
                                                                text: String(month)
                                                                horizontalAlignment: Text.AlignHCenter
                                                                color: root.textSecondary
                                                                font.pixelSize: 8
                                                            }
                                                        }
                                                    }
                                                }

                                                Rectangle {
                                                    visible: root.tokenHoveredMonthlyIndex >= 0
                                                    anchors.right: parent.right
                                                    anchors.top: parent.top
                                                    anchors.margins: 8
                                                    radius: 8
                                                    color: Qt.rgba(15/255, 23/255, 42/255, 0.86)
                                                    border.color: Qt.rgba(1, 1, 1, 0.24)
                                                    implicitWidth: 170
                                                    implicitHeight: 54

                                                    Column {
                                                        id: monthlyTooltip
                                                        anchors.fill: parent
                                                        anchors.margins: 8
                                                        spacing: 2
                                                        property var hoveredMonthly: root.tokenHoveredMonthlyIndex >= 0 && root.tokenHoveredMonthlyIndex < tokenMonthlyModel.count
                                                            ? tokenMonthlyModel.get(root.tokenHoveredMonthlyIndex) : null
                                                        Text { text: monthlyTooltip.hoveredMonthly ? (monthlyTooltip.hoveredMonthly.year + "-" + monthlyTooltip.hoveredMonthly.month) : ""; color: "white"; font.pixelSize: 10; font.bold: true }
                                                        Text { text: "Total: " + root.formatTokenValue(monthlyTooltip.hoveredMonthly ? monthlyTooltip.hoveredMonthly.total : 0); color: "#FDBA74"; font.pixelSize: 10 }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 190
                                        radius: 12
                                        color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                        border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            spacing: 6

                                            Text {
                                                text: "历年 Token 用量"
                                                color: root.textSecondary
                                                font.pixelSize: 10
                                            }

                                            Rectangle {
                                                id: tokenYearlyPlot
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                radius: 8
                                                color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.04)
                                                border.color: Qt.rgba(42 / 255, 125 / 255, 255 / 255, 0.18)

                                                Row {
                                                    anchors.fill: parent
                                                    anchors.margins: 6
                                                    spacing: 6

                                                    Repeater {
                                                        model: tokenYearlyModel

                                                        delegate: Column {
                                                            required property string label
                                                            required property int total
                                                            width: Math.max(34, (tokenYearlyPlot.width - 12 - Math.max(0, tokenYearlyModel.count - 1) * 6) / Math.max(1, tokenYearlyModel.count))
                                                            height: tokenYearlyPlot.height - 12
                                                            spacing: 2

                                                            Item {
                                                                width: parent.width
                                                                height: parent.height - 16

                                                                MouseArea {
                                                                    anchors.fill: parent
                                                                    hoverEnabled: true
                                                                    onEntered: root.tokenHoveredYearlyIndex = index
                                                                    onExited: {
                                                                        if (root.tokenHoveredYearlyIndex === index) {
                                                                            root.tokenHoveredYearlyIndex = -1
                                                                        }
                                                                    }
                                                                }

                                                                Rectangle {
                                                                    anchors.left: parent.left
                                                                    anchors.right: parent.right
                                                                    anchors.bottom: parent.bottom
                                                                    radius: 2
                                                                    height: Math.max(2, (total / Math.max(1, root.tokenYearlyMax)) * (parent.height - 2))
                                                                    color: Qt.rgba(237 / 255, 137 / 255, 54 / 255, 0.74)
                                                                }
                                                            }

                                                            Text {
                                                                width: parent.width
                                                                text: label
                                                                horizontalAlignment: Text.AlignHCenter
                                                                color: root.textSecondary
                                                                font.pixelSize: 8
                                                            }
                                                        }
                                                    }
                                                }

                                                Rectangle {
                                                    visible: root.tokenHoveredYearlyIndex >= 0
                                                    anchors.right: parent.right
                                                    anchors.top: parent.top
                                                    anchors.margins: 8
                                                    radius: 8
                                                    color: Qt.rgba(15/255, 23/255, 42/255, 0.86)
                                                    border.color: Qt.rgba(1, 1, 1, 0.24)
                                                    implicitWidth: 170
                                                    implicitHeight: 54

                                                    Column {
                                                        id: yearlyTooltip
                                                        anchors.fill: parent
                                                        anchors.margins: 8
                                                        spacing: 2
                                                        property var hoveredYearly: root.tokenHoveredYearlyIndex >= 0 && root.tokenHoveredYearlyIndex < tokenYearlyModel.count
                                                            ? tokenYearlyModel.get(root.tokenHoveredYearlyIndex) : null
                                                        Text { text: yearlyTooltip.hoveredYearly ? yearlyTooltip.hoveredYearly.label : ""; color: "white"; font.pixelSize: 10; font.bold: true }
                                                        Text { text: "Total: " + root.formatTokenValue(yearlyTooltip.hoveredYearly ? yearlyTooltip.hoveredYearly.total : 0); color: "#FDBA74"; font.pixelSize: 10 }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 280
                                        radius: 12
                                        color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.22)
                                        border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            spacing: 6

                                            RowLayout {
                                                Layout.fillWidth: true

                                                Text {
                                                    text: "最近使用记录（时间/Token）"
                                                    color: root.textSecondary
                                                    font.pixelSize: 10
                                                }

                                                Item { Layout.fillWidth: true }

                                                GlassComboBox {
                                                    id: tokenResetYearSelector
                                                    model: tokenYearOptionsModel
                                                    textRole: "label"
                                                    implicitWidth: 90
                                                    onActivated: {
                                                        var selectedYear = tokenYearOptionsModel.get(currentIndex)
                                                        if (selectedYear && selectedYear.year) {
                                                            root.tokenResetYear = Number(selectedYear.year)
                                                        }
                                                    }
                                                }

                                                GlassComboBox {
                                                    id: tokenResetMonthSelector
                                                    model: tokenMonthOptionsModel
                                                    textRole: "label"
                                                    implicitWidth: 72
                                                    onActivated: {
                                                        var selectedMonth = tokenMonthOptionsModel.get(currentIndex)
                                                        if (selectedMonth && selectedMonth.value) {
                                                            root.tokenResetMonth = Number(selectedMonth.value)
                                                        }
                                                    }
                                                }

                                                DangerButton {
                                                    text: "重置月"
                                                    onClicked: root.requestTokenReset("month")
                                                }

                                                DangerButton {
                                                    text: "重置年"
                                                    onClicked: root.requestTokenReset("year")
                                                }

                                                DangerButton {
                                                    text: "全部重置"
                                                    onClicked: root.requestTokenReset("all")
                                                }
                                            }

                                            Rectangle {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                radius: 10
                                                color: Qt.rgba(240 / 255, 242 / 255, 248 / 255, 0.20)
                                                border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                                ListView {
                                                    anchors.fill: parent
                                                    anchors.margins: 6
                                                    clip: true
                                                    spacing: 4
                                                    model: tokenRecentEventsModel

                                                    delegate: Rectangle {
                                                        required property string timestamp
                                                        required property int prompt
                                                        required property int completion
                                                        required property int total
                                                        width: ListView.view.width
                                                        implicitHeight: 30
                                                        radius: 8
                                                        color: Qt.rgba(235 / 255, 238 / 255, 245 / 255, 0.24)
                                                        border.color: Qt.rgba(195 / 255, 198 / 255, 212 / 255, 0.18)

                                                        RowLayout {
                                                            anchors.fill: parent
                                                            anchors.leftMargin: 8
                                                            anchors.rightMargin: 8

                                                            Text {
                                                                Layout.preferredWidth: 175
                                                                text: timestamp ? String(timestamp).replace("T", " ").slice(0, 19) : ""
                                                                color: root.textSecondary
                                                                font.pixelSize: 10
                                                                elide: Text.ElideRight
                                                            }

                                                            Text {
                                                                Layout.fillWidth: true
                                                                text: "P:" + prompt + "  C:" + completion
                                                                color: root.textSecondary
                                                                font.pixelSize: 10
                                                                elide: Text.ElideRight
                                                            }

                                                            Text {
                                                                text: String(total)
                                                                color: root.textPrimary
                                                                font.pixelSize: 11
                                                                font.bold: true
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
