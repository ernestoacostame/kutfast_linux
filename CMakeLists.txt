cmake_minimum_required(VERSION 3.21)
project(kutfast VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Network
    Multimedia
    Quick
    QuickControls2
    Widgets
)

qt_standard_project_setup(REQUIRES 6.5)

# Silencia el warning QTP0004 (qmldir por subdirectorio)
if(COMMAND qt_policy)
    qt_policy(SET QTP0004 OLD)
endif()

set(SOURCES
    src/main.cpp
    src/app/AppController.cpp
    src/api/KutPodClient.cpp
    src/audio/AudioProcessor.cpp
    src/audio/AudioRecorder.cpp
    src/store/DraftStore.cpp
    src/store/TemplateStore.cpp
    src/store/PodcastStore.cpp
)

set(HEADERS
    src/app/AppController.h
    src/api/KutPodClient.h
    src/api/ApiTypes.h
    src/audio/AudioProcessor.h
    src/audio/AudioRecorder.h
    src/store/DraftStore.h
    src/store/TemplateStore.h
    src/store/PodcastStore.h
)

qt_add_executable(kutfast
    ${SOURCES}
    ${HEADERS}
)

# El módulo QML genera un directorio "kutfast/" en build/;
# renombramos solo el binario resultante para evitar la colisión.
set_target_properties(kutfast PROPERTIES OUTPUT_NAME "kutfast-bin")

qt_add_qml_module(kutfast
    URI kutfast
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/pages/DashboardPage.qml
        qml/pages/PublishPage.qml
        qml/pages/EpisodesPage.qml
        qml/pages/DraftsPage.qml
        qml/pages/TemplatesPage.qml
        qml/pages/PodcastsPage.qml
        qml/pages/SettingsPage.qml
        qml/components/EpisodeCard.qml
        qml/components/AudioRecorderWidget.qml
        qml/components/TemplatePickerDialog.qml
        qml/components/PodcastSwitcher.qml
        qml/components/StatusBanner.qml
        qml/components/SideBar.qml
        qml/components/Theme.qml
    RESOURCES
        resources/icons/kutfast.svg
)

target_link_libraries(kutfast PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Multimedia
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
)

# Install
include(GNUInstallDirs)
install(TARGETS kutfast
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
