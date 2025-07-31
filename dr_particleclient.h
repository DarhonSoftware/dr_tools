//Release 2
#ifndef DR_SPARKCLIENT_H
#define DR_SPARKCLIENT_H

#include <QEventLoop>
#include <QNetworkReply>
#include <QObject>

struct SDevice
{
    QString sDeviceId;
    QString sName;
    QDateTime DateLastHeard;
    bool bConnected;
};

class CParticleClient : public QObject
{
    Q_OBJECT

public:
    enum Action { ActionListDevices, ActionVariable, ActionFunction, ActionSubscribeEvent, ActionPublishEvent };

    enum Event {
        EventGlobal, //Subscribe to the firehose of public events, plus private events published by devices one owns
        EventOwn,    //Subscribe to all events, public and private, published by devices one owns
        EventDevice  //Subscribe to events from one specific device
    };

private:
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)

    //Control parameters
    bool m_bRunning;
    QString m_sAccessToken;
    Action m_iAction;

    //Control subscribed events
    QNetworkReply *m_pFirstReply;
    QByteArray m_BAEvent;
    QByteArray m_BAData;

    //Response parameters
    QList<SDevice> m_lstDevices;
    QVariant m_VariableValue;
    qint32 m_iFunctionValue;
    QString m_sLastError;

    //Local objects
    QNetworkAccessManager m_Manager;
    QEventLoop m_EventLoop;
    QList<QNetworkReply *> m_lstReplies;

public:
    explicit CParticleClient(QObject *pObject = 0);
    ~CParticleClient();

    //Property implementation
    void setRunning(bool bRunning)
    {
        m_bRunning = bRunning;
        emit runningChanged();
    }
    bool running() const { return m_bRunning; }

    //Initialization
    void setToken(const QString &sToken);

    //Services
    bool listDevices();
    bool variable(const QString &sDeviceId, const QString &sVariable);
    bool function(const QString &sDeviceId, const QString &sFunction, const QString &sArgument = QString());
    bool publishEvent(const QString &sEventName, const QString &sData, bool bPrivate = true, int iTtl = 60);
    bool subscribeEvent(Event iType, const QString &sEventName, const QString &sDeviceId = QString());

    //Responses
    QList<SDevice> *responseDevices() { return &m_lstDevices; }
    QVariant responseVariable() const { return m_VariableValue; }
    qint32 responseFunction() const { return m_iFunctionValue; }
    QString lastError() const { return m_sLastError; }

signals:
    void runningChanged();
    void subscribedEvent(const QString &sEvent, const QString &sData);

private slots:
    void replyFinished(QNetworkReply *pReply);
    void eventReceived();
};

#endif // DR_SPARKCLIENT_H
