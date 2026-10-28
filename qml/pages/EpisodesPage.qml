import kutfast
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: root
    background: Rectangle { color: theme.bgBase }

    Component.onCompleted: App.refreshEpisodes()

    header: ToolBar {
        Material.background: theme.bgHeader
        RowLayout {
            anchors { fill: parent; leftMargin: 16; rightMargin: 8 }
            spacing: 10

            Rectangle {
                width: 28; height: 28; radius: 6
                color: theme.bgInput; clip: true
                border.color: theme.cardBorder; border.width: 1
                visible: (App.podcastInfo ? (App.podcastInfo.coverUrl || "") : "") !== ""
                Image {
                    anchors.fill: parent
                    source: App.podcastInfo ? (App.podcastInfo.coverUrl || "") : ""
                    fillMode: Image.PreserveAspectCrop
                }
            }

            Label {
                text: "Episodios · " + (App.podcastInfo && App.podcastInfo.title ? App.podcastInfo.title : App.activePodcast)
                font.pixelSize: 15; font.weight: Font.DemiBold; color: "white"
                font.family: theme.fontFamily
                Layout.fillWidth: true
            }
            Label {
                text: App.episodes.length + " publicados"
                color: theme.textMutedOnDark; font.pixelSize: 12
                font.family: theme.fontMono
            }
            ToolButton {
                text: "↻"; font.pixelSize: 18
                onClicked: App.refreshEpisodes()
                ToolTip.text: "Actualizar"; ToolTip.visible: hovered
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            padding: 16
            spacing: 8

            Label {
                visible: App.episodes.length === 0 && !App.busy
                text: "No hay episodios publicados."
                color: theme.textMuted; font.pixelSize: 13
                font.family: theme.fontFamily
                anchors.horizontalCenter: parent.horizontalCenter
                topPadding: 40
            }

            Repeater {
                model: App.episodes
                EpisodeCard {
                    required property var modelData
                    episode: modelData
                    width: parent.width - 32
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }
}
