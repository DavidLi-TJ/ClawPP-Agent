cmake_minimum_required(VERSION 3.16)

project(ClawPP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network Sql Widgets)

qt_standard_project_setup(REQUIRES 6.5)

# ========== Sources ==========

set(PROJECT_SOURCES
    src/main.cpp
    src/types.cpp

    # Headers with Q_OBJECT that need MOC processing
    src/agent/i_agent_core.h
    src/memory/i_memory_system.h
    src/provider/i_llm_provider.h
    src/tool/i_tool.h

    # Agent
    src/agent/agent_profile.cpp
    src/agent/agent_profile_manager.cpp
    src/agent/context_builder.cpp
    src/agent/react_agent_core.cpp

    # Application
    src/application/agent_orchestrator.cpp
    src/application/agent_service.cpp
    src/application/session_manager.cpp
    src/application/session_thread.cpp
    src/application/template_loader.cpp

    # Infrastructure
    src/infrastructure/config/config_manager.cpp
    src/infrastructure/database/conversation_store.cpp
    src/infrastructure/database/database_manager.cpp
    src/infrastructure/database/session_store.cpp
    src/infrastructure/event/event_bus.cpp
    src/infrastructure/logging/logger.cpp
    src/infrastructure/network/external_platform_manager.cpp
    src/infrastructure/network/http_client.cpp
    src/infrastructure/network/sse_parser.cpp
    src/infrastructure/runner/runner_manager.cpp

    # Memory
    src/memory/conversation_history.cpp
    src/memory/memory_manager.cpp

    # Permission
    src/permission/bash_analyzer.cpp
    src/permission/permission_manager.cpp

    # Provider
    src/provider/anthropic_provider.cpp
    src/provider/gemini_provider.cpp
    src/provider/openai_provider.cpp
    src/provider/provider_manager.cpp

    # QML Backend
    src/qml/qml_backend.cpp
    src/qml/qml_models.cpp

    # Skill
    src/skill/skill.cpp
    src/skill/skill_manager.cpp

    # Tool
    src/tool/i_tool.cpp
    src/tool/tool_executor.cpp
    src/tool/tool_registry.cpp
    src/tool/tools/edit_file_tool.cpp
    src/tool/tools/filesystem_tool.cpp
    src/tool/tools/list_directory_tool.cpp
    src/tool/tools/network_tool.cpp
    src/tool/tools/read_file_tool.cpp
    src/tool/tools/search_files_tool.cpp
    src/tool/tools/shell_tool.cpp
    src/tool/tools/subagent_tool.cpp
    src/tool/tools/system_tool.cpp
    src/tool/tools/write_file_tool.cpp
)

# ========== QML Resources ==========

set(QML_RESOURCES
    qml/qml.qrc
    qml/ClawPP.qrc
    resources/resources.qrc
)

# ========== Executable ==========

qt_add_executable(ClawPP WIN32
    ${PROJECT_SOURCES}
    ${QML_RESOURCES}
    app_icon.rc
)

# ========== Include Directories ==========

target_include_directories(ClawPP PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ========== Libraries ==========

target_link_libraries(ClawPP PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Sql
    Qt6::Widgets
)

# ========== Compile Definitions ==========

target_compile_definitions(ClawPP PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
)

# ========== Install ==========

install(TARGETS ClawPP
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET ClawPP
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})
