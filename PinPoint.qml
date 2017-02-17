import QtQuick 2.6
import QtQuick.Window 2.2

Rectangle {
    property Item container

    x: parent.width / 2
    y: parent.height / 2
    visible: false
    id: rect
    width: 10
    height: 10
    color: "red"

    MouseArea {
        anchors.fill: parent
        drag.target: rect
        drag.axis: Drag.XAxis | Drag.YAxis
        drag.minimumX: 0
        drag.maximumX: container !== null ? container.width - rect.width : 0
        drag.minimumY: 0
        drag.maximumY: container !== null ? container.height - rect.height : 0
    }

}
