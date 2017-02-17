import QtMultimedia 5.5
import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.2

import carfilter 1.0


Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Car Counter")

    FileDialog {
        id: openVideo
        title: "Choose a videofile"
        onAccepted: {
            player.stop()
            player.source = fileUrls[0];
            player.play()
        }
    }

    Row {
        id: tools
        Button {
            text: "Open Video"
            onClicked: {
                openVideo.open();
            }
        }
        Button {
            MessageDialog {
                id: dialogTooManyPoints
                title: "Too many points"
                icon: StandardIcon.Warning
                text: "You have added too many points already."
                standardButtons: StandardButton.Ok
            }
            id: addpoint
            text: "Add Point"
            onClicked: {
                for (var i = 0; i < output.children.length; ++i) {
                    var child = output.children[i];
                    if (!child.visible) {
                        child.visible = true;
                        return;
                    }
                }
                dialogTooManyPoints.open();
            }
        }
        Button {
            id: deletepoint
            text: "Delete Point"
            onClicked: {
                var lastVisible = null
                for (var i = 0; i < output.children.length; ++i) {
                    var child = output.children[i];
                    if (child.visible)
                        lastVisible = child;
                }
                if (lastVisible != null)
                    lastVisible.visible = false;
            }
        }
    }

    MediaPlayer {
        id: player
        autoPlay: true
    }

    VideoOutput {
        id: output
        source: player
        filters: [ carfilter ]
        anchors.left: parent.left
        anchors.top: tools.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        PinPoint { container: output }
        PinPoint { container: output }
        PinPoint { container: output }
        PinPoint { container: output }
        PinPoint { container: output }
        PinPoint { container: output }
        PinPoint { container: output }
        PinPoint { container: output }
        PinPoint { container: output }
        PinPoint { container: output }
    }

    CarFilter {
        id: carfilter
    }

    /*
    MainForm {
        anchors.fill: parent
        mouseArea.onClicked: {
            console.log(qsTr('Clicked on background. Text: "' + textEdit.text + '"'))
        }
    }
    */
}
