import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import kutfast

ApplicationWindow {
    id: root
    visible: true
    width:  960
    height: 700
    minimumWidth:  800
    minimumHeight: 600
    title: "KutFast"

    // Theme instanciado aquí con id: theme
    // Al estar en la raíz, todos los QML hijos lo ven como "Theme"
    Theme {
        id: theme
    }

    Material.theme:  theme.dark ? Material.Dark : Material.Light
    Material.accent: Material.DeepPurple

    background: Rectangle { color: theme.bgBase }

    function openPublishWithAudio(path) {
        stack.replace(publishPage)
        Qt.callLater(() => {
            if (stack.currentItem && stack.currentItem.setAudioFile) {
                stack.currentItem.setAudioFile(path)
            }
        })
    }

    Component.onCompleted: {
        if (!App.configured) {
            stack.replace(settingsPage)
        } else if (App.initialAudioFile !== "") {
            openPublishWithAudio(App.initialAudioFile)
        } else {
            stack.replace(dashboardPage)
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        SideBar {
            id: sidebar
            visible: App.configured
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            onNavigate: (page) => navigateTo(page)
        }

        StackView {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: dashboardPage
            clip: true
            pushEnter:    Transition { PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
            pushExit:     Transition { PropertyAnimation { property: "opacity"; from: 1; to: 0; duration: 100 } }
            replaceEnter: Transition { PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
            replaceExit:  Transition { PropertyAnimation { property: "opacity"; from: 1; to: 0; duration: 100 } }
        }
    }

    StatusBanner {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        message: App.statusMessage
        isError: App.statusIsError
        onDismissed: App.clearStatus()
    }

    Rectangle {
        anchors.fill: parent
        color: theme.overlay
        visible: App.busy
        Column {
            anchors.centerIn: parent
            spacing: 16
            BusyIndicator { anchors.horizontalCenter: parent.horizontalCenter; running: true }
            Label {
                text: App.conversionProgress > 0 && App.conversionProgress < 100
                      ? "Convirtiendo audio… %1%".arg(App.conversionProgress)
                      : App.uploadProgress > 0 ? "Subiendo… %1%".arg(App.uploadProgress)
                      : "Procesando…"
                color: "white"; font.pixelSize: 14
                anchors.horizontalCenter: parent.horizontalCenter
            }
            ProgressBar {
                width: 280
                anchors.horizontalCenter: parent.horizontalCenter
                value: App.conversionProgress > 0 && App.conversionProgress < 100
                       ? App.conversionProgress / 100 : App.uploadProgress / 100
                visible: App.conversionProgress > 0 || App.uploadProgress > 0
                Material.accent: Material.LightGreen
            }
        }
    }

    Component {
        id: dashboardPage
        DashboardPage {
            onOpenLocalDraft: (draftId) => {
                stack.replace(publishPage)
                Qt.callLater(() => {
                    if (stack.currentItem && stack.currentItem.loadDraft)
                        stack.currentItem.loadDraft(draftId)
                })
            }
            onOpenPublish: stack.replace(publishPage)
            onOpenDrafts: stack.replace(draftsPage)
        }
    }
    Component { id: publishPage;   PublishPage   {} }
    Component { id: episodesPage;  EpisodesPage  {} }
    Component { 
        id: draftsPage
        DraftsPage {
            onOpenLocalDraft: (draftId) => {
                stack.replace(publishPage)
                Qt.callLater(() => {
                    if (stack.currentItem && stack.currentItem.loadDraft) {
                        console.log("Loading draft in PublishPage:", draftId)
                        stack.currentItem.loadDraft(draftId)
                    }
                })
            }
        }
    }
    Component { id: templatesPage; TemplatesPage {} }
    Component { id: podcastsPage;  PodcastsPage  {} }
    Component { id: settingsPage;  SettingsPage  {} }

    function navigateTo(page) {
        const map = {
            dashboard: dashboardPage, publish: publishPage,
            episodes: episodesPage,   drafts: draftsPage,
            templates: templatesPage, podcasts: podcastsPage,
            settings: settingsPage
        }
        if (map[page]) stack.replace(map[page])
    }

    // ── Global Drag & Drop ───────────────────────────────────
    DropArea {
        id: globalDropArea
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: (drop) => {
            if (drop.hasUrls && drop.urls.length > 0) {
                let path = drop.urls[0].toString().replace("file://", "")
                openPublishWithAudio(path)
            }
        }
    }

    // Overlay visual durante el arrastre
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(15/255, 17/255, 23/255, 0.88)
        visible: globalDropArea.containsDrag
        z: 99
        border.color: theme.accentLight
        border.width: 3

        Column {
            anchors.centerIn: parent
            spacing: 12
            Label {
                text: "🎙"
                font.pixelSize: 48
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label {
                text: "Suelta el archivo de audio para publicarlo"
                font.pixelSize: 18
                font.bold: true
                color: "white"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label {
                text: "MP3, WAV, FLAC, OGG, M4A, OPUS, WEBM…"
                font.pixelSize: 13
                color: theme.textMuted
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    Connections {
        target: App
        function onConfiguredChanged() {
            if (App.configured) { stack.replace(dashboardPage); App.refreshEpisodes() }
        }
        function onInitialAudioFileChanged() {
            if (App.initialAudioFile !== "") {
                openPublishWithAudio(App.initialAudioFile)
            }
        }
    }
}
