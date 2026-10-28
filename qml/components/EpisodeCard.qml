import kutfast
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtMultimedia

Rectangle {
    id: root
    property var episode: ({})
    signal publishDraftRequested(int episodeId)

    height: contentCol.implicitHeight + 20
    radius: theme.radiusMd
    color: hovered ? theme.cardHover : theme.cardBg
    border.color: theme.cardBorder
    border.width: 1

    property bool hovered: false
    Behavior on color { ColorAnimation { duration: 80 } }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: root.hovered = true
        onExited:  root.hovered = false
    }

    MediaPlayer {
        id: player
        audioOutput: AudioOutput {}
        source: episode.audioUrl || ""
    }

    ColumnLayout {
        id: contentCol
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // Número de episodio - pastilla minimalista
            Rectangle {
                width: 34; height: 26; radius: 5
                color: episode.isDraft ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.12)
                                       : Qt.rgba(theme.success.r, theme.success.g, theme.success.b, 0.12)
                Label {
                    anchors.centerIn: parent
                    text: episode.episodeNumber > 0 ? "#" + episode.episodeNumber : "–"
                    color: episode.isDraft ? theme.accent : theme.success
                    font.pixelSize: 11; font.weight: Font.DemiBold
                    font.family: theme.fontMono
                }
            }

            // Portada del Podcast cargado en KutFast
            Rectangle {
                width: 38; height: 38; radius: 6
                color: theme.bgInput
                clip: true
                border.color: theme.cardBorder; border.width: 1

                Image {
                    id: podCoverImg
                    anchors.fill: parent
                    source: episode.coverUrl || (App.podcastInfo ? App.podcastInfo.coverUrl : "") || ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    visible: status === Image.Ready
                }

                Label {
                    anchors.centerIn: parent
                    text: "🎙"
                    font.pixelSize: 16
                    visible: !podCoverImg.visible
                }
            }

            Column {
                Layout.fillWidth: true
                spacing: 2
                Label {
                    text: episode.title || "(sin título)"
                    color: theme.textPrimary
                    font.pixelSize: 13; font.weight: Font.DemiBold
                    font.family: theme.fontFamily
                    elide: Text.ElideRight
                    width: parent.width
                }
                Label {
                    visible: episode.seasonNumber > 0
                    text: "Temporada " + episode.seasonNumber
                    color: theme.textMuted
                    font.pixelSize: 10
                    font.family: theme.fontFamily
                }
            }

            // Badge estado
            Rectangle {
                radius: 4
                width: badgeLabel.implicitWidth + 12
                height: 20
                color: episode.isDraft
                       ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.15)
                       : Qt.rgba(theme.success.r, theme.success.g, theme.success.b, 0.15)
                Label {
                    id: badgeLabel
                    anchors.centerIn: parent
                    text:  episode.isDraft ? "BORRADOR" : "PUBLICADO"
                    color: episode.isDraft ? theme.accent : theme.success
                    font.pixelSize: 9; font.weight: Font.DemiBold; font.letterSpacing: 0.6
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: {
                    let raw = episode.publishedAt || episode.createdAt || ""
                    if (!raw) return ""
                    let d = new Date(raw)
                    return isNaN(d) ? raw : d.toLocaleDateString(Qt.locale(), "d MMM yyyy")
                }
                color: theme.textMuted
                font.pixelSize: 10
            }

            Item { Layout.fillWidth: true }

            Button {
                visible: (episode.audioUrl || "").length > 0
                text: player.playbackState === MediaPlayer.PlayingState ? "⏸ Pausar" : "▶ Escuchar"
                flat: true; font.pixelSize: 11
                Material.foreground: theme.accentLight
                onClicked: {
                    if (player.playbackState === MediaPlayer.PlayingState) {
                        player.pause()
                    } else {
                        player.play()
                    }
                }
            }

            Button {
                visible: episode.isDraft && episode.id > 0
                text: "Publicar"
                flat: true; font.pixelSize: 11
                Material.foreground: theme.accent
                onClicked: root.publishDraftRequested(episode.id)
            }
        }
    }
}
