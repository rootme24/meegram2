import QtQuick 1.1
import com.nokia.meego 1.0
import com.nokia.extras 1.1
import MyComponent 1.0

Page {
    id: root

    property string phoneNumber: ""
    property variant type
    property variant nextType: null
    property int timeout: 0

    signal cancelClicked

    Flickable {
        id: flickable
        anchors.fill: parent
        anchors.margins: 16
        contentHeight: contentColumn.height

        Column {
            id: contentColumn

            width: flickable.width
            height: childrenRect.height

            spacing: 16

            Label {
                id: title
                text: app.getString("YourCode") + app.emptyString
                font.pixelSize: 40
            }
            Rectangle {
                color: "#b2b2b4"
                height: 1
                width: flickable.width
            }

            // Code
            Column {
                id: codeEnterColumn

                width: parent.width
                spacing: 16

                Label {
                    id: codeTitle
                    text: getCodeTitle()
                }

                Column {
                    width: parent.width
                    spacing: 20

                    TextField {
                        id: code
                        width: parent.width
                        inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
                        placeholderText: app.getString("Code") + app.emptyString

                        onTextChanged: {
                            if(text.length >= getCodeLength()) {
                                authorization.loading = true
                                authorization.checkCode(code.text)
                            }
                        }
                    }

                    Label {
                        id: codeType
                        font.pixelSize: 24
                        width: parent.width
                        text: getCodeSubtitle()
                    }

                    Label {
                        id: isNextTypeSms

                        anchors.horizontalCenter: parent.horizontalCenter

                        font.pixelSize: 24
                        font.underline: true

                        color: "#0088cc"
                        text: app.getString("DidNotGetTheCodeSms") + app.emptyString

                        visible: nextType.type === "authenticationCodeTypeSms"

                        MouseArea {
                            anchors.fill: parent
                            onClicked: authorization.resendCode()
                        }
                    }

                    Row {
                        id: codeTextRow
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 4
                        Label {
                            id: nextTypeLabel
                            horizontalAlignment: Text.AlignHCenter
                            font.pointSize: 24
                            text: getCodeNextTypeString()
                        }
                        Label {
                            id: codeTimeText
                            horizontalAlignment: Text.AlignHCenter
                            font.pointSize: 24
                            color: theme.selectionColor // "#ffcc00"
                        }
                        visible: codeTimeText.text !== ""
                    }

                    Timer {
                        id: codeExpireTimer
                        interval: 1000
                        repeat: true
                        onTriggered: {
                            timeout = timeout - 1000;
                            codeTimeText.text = authorization.formatTime(timeout / 1000);
                            if (timeout === 0) {
                                codeExpireTimer.stop()
                                codeTextRow.visible = false;
                                authorization.resendCode()
                            }
                        }
                    }
                }
            }
        }
    }

    tools: ToolBarLayout {
        ToolButtonRow {
            ToolButton {
                text: app.getString("Next") + app.emptyString
                onClicked: {
                    authorization.loading = true;
                    authorization.checkCode(code.text);
                }
            }
            ToolButton {
                text: app.getString("Cancel") + app.emptyString
                onClicked: {
                    authorization.loading = false;
                    root.cancelClicked();
                }
            }
        }
    }

    function getCodeTitle() {
        switch (type.type) {
        case "authenticationCodeTypeTelegramMessage":
            return app.getString("SentAppCodeTitle");
        case "authenticationCodeTypeCall":
        case "authenticationCodeTypeSms":
            return app.getString("SentSmsCodeTitle");
        default:
            return app.getString("Title");
        }
    }

    function getCodeSubtitle() {
        switch (type.type) {
        case "authenticationCodeTypeCall":
            return app.getString("SentCallCode").arg(phoneNumber);
        case "authenticationCodeTypeFlashCall":
            return app.getString("SentCallOnly").arg(phoneNumber);
        case "authenticationCodeTypeSms":
            return app.getString("SentSmsCode").arg(phoneNumber);
        case "authenticationCodeTypeTelegramMessage":
            return app.getString("SentAppCode");
        default:
            return "";
        }
    }

    function getCodeNextTypeString() {
        switch (nextType.type) {
        case "authenticationCodeTypeCall":
            return app.getString("CallText");
        case "authenticationCodeTypeSms":
            return app.getString("SmsText");
        default:
            return "";
        }
    }

    function getCodeLength() {
        return type.length || 0;
    }

    onTimeoutChanged: {
        codeExpireTimer.start()
        codeTimeText.text = authorization.formatTime(timeout / 1000)
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: authorization.loading
        visible: authorization.loading
        platformStyle: BusyIndicatorStyle { size: "large" }
    }
}
