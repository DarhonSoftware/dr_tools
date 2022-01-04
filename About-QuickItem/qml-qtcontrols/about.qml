import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Window 2.0
import com.darhon 1.0

Page {
    id: id_item

    About {id: id_about}

    header: ToolBar {
        id: id_toolbar
        RowLayout {
            id: id_layout
            ToolButton {
                id: id_backButton
                Layout.preferredWidth: (id_item.width-id_layout.spacing*4)/5
                Image {
                    height: id_backButton.height*0.5
                    width: id_backButton.height*0.5
                    anchors.centerIn: parent
                    source: id_about.iconBack
                    smooth: true
                }

                onClicked: id_stack.pop()
            }
            ToolButton {
                id: id_button
                text: qsTr("Code")
                visible: id_about.codesEnabled
                Layout.preferredWidth: (id_item.width-id_layout.spacing*4)/5
                onClicked: {
                    id_main.visible=!id_main.visible
                    id_codes.visible=!id_codes.visible
                }
            }
        }
    }

    Label { id: id_labelRef; text: "W"; visible: false }

    Column {
        id: id_main
        anchors.centerIn: parent
        width: parent.width-id_labelRef.height*2
        spacing: id_labelRef.height

        //ICON
        Image {
            id: id_iconAbout
            width: id_labelRef.height*4
            height: id_labelRef.height*4
            anchors.horizontalCenter: parent.horizontalCenter
            source: id_about.iconAbout
            visible: id_item.Screen.primaryOrientation===Qt.PortraitOrientation
        }

        //Application name
        Column {
            anchors.horizontalCenter: parent.horizontalCenter

            Label {
                id: id_appName
                anchors.horizontalCenter: parent.horizontalCenter
                text: id_about.appName
                font.bold: true
            }

            Label {
                id: id_version
                anchors.horizontalCenter: parent.horizontalCenter
                text: "( " + id_about.version + " )"
                font.pointSize: id_labelRef.font.pointSize*0.8
            }
        }

        //Short sentence
        Label {
            id: id_caption
            anchors.horizontalCenter: parent.horizontalCenter
            text: id_about.caption
        }

        //Long sentence
        Label {
            id: id_description
            width: parent.width
            horizontalAlignment: Text.AlignJustify
            text: id_about.description
            wrapMode: Text.WordWrap
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5

            Image {
                id: id_iconDarhon
                width: id_labelRef.height*2
                height: id_labelRef.height*2
                anchors.verticalCenter: parent.verticalCenter
                source: id_about.iconDarhon
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter

                Label {
                    id: id_copyright
                    text: id_about.copyright
                    font.pointSize: id_labelRef.font.pointSize*0.8
                }
                Label {
                    id: id_support
                    text: id_about.support
                    font.pointSize: id_labelRef.font.pointSize*0.8
                }
                Label {
                    id: id_privacy
                    text: id_about.privacy
                    font.pointSize: id_labelRef.font.pointSize*0.8
                    onLinkActivated: Qt.openUrlExternally(link)
                }
            }
        }
    }

    Column {
        id: id_codes
        anchors.centerIn: parent
        width: parent.width
        spacing: id_labelRef.height
        visible: false

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Application code")
                font.bold: true
            }
            Label {
                id: id_appCode
                anchors.horizontalCenter: parent.horizontalCenter
                text: id_about.appCode
            }
        }


        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Activation code")
                font.bold: true
            }
            Label {
                id: id_actCode
                anchors.horizontalCenter: parent.horizontalCenter
                text: id_about.actCode
            }
            TextField {
                id: id_inputActCode
                anchors.horizontalCenter: parent.horizontalCenter
                width: id_appCode.width+id_labelRef.width*2
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                inputMask: "HHHH-HHHH-HHHH-HHHH;_"
                text: id_about.actCode
            }
        }

        Button {
            id: id_buttonSave
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Register")
            onClicked: {
                id_about.save(id_inputActCode.text)
            }
        }


        states: [
            State {
                name: "SILVER"
                when: id_about.actCode==""
                PropertyChanges { target: id_actCode; visible: false }
                PropertyChanges { target: id_inputActCode; visible: true }
                PropertyChanges { target: id_buttonSave; visible: true }
            },
            State {
                name: "GOLD"
                when: id_about.actCode!=""
                PropertyChanges { target: id_actCode; visible: true }
                PropertyChanges { target: id_inputActCode; visible: false }
                PropertyChanges { target: id_buttonSave; visible: false }
            }
        ]
    }

    Keys.onBackPressed: {
        id_backButton.clicked()
    }

    Component.onCompleted: {
        id_item.focus=true
    }
}
