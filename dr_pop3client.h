//Release 2
#ifndef DR_CPOP3CLIENT_H
#define DR_CPOP3CLIENT_H

#include <QObject>
#include <QQueue>
#include <QTcpSocket>

class CPop3Client : public QObject
{
    Q_OBJECT

public:
    enum AuthMethod { AuthPlain, AuthAPOP };
    enum ConnectionType { TcpConnection, SslConnection, TlsConnection };
    enum Error {
        ErrorNoError,                   // No error occurred
        ErrorSocketTimeoutConnection,   // Socket timeout while waiting for connection
        ErrorSocketTimeoutReadyRead,    // Socket timeout while waiting for data to read from buffer
        ErrorSocketTimeoutBytesWritten, // Socket timeout while waiting to write data to buffer
        ErrorSocketInUse,               // Socket is already connected
        ErrorSocketInvalid,             // Socket is not connected
        ErrorSocketEncryption,          // Socket failed to complete handshake for encryption
        ErrorReplyConnection,           // Server reply -ERR after connection attempt
        ErrorReplySTLS,                 // Server reply -ERR after command STLS
        ErrorReplyAPOP,                 // Server reply -ERR after command APOP
        ErrorReplyUSER,                 // Server reply -ERR after command USER
        ErrorReplyPASS,                 // Server reply -ERR after command PASS
        ErrorReplySTAT,                 // Server reply -ERR after command STAT
        ErrorReplyRETR,                 // Server reply -ERR after command RETR
        ErrorReplyDELE,                 // Server reply -ERR after command RETR
    };

private:
    QTcpSocket *m_pSocket;
    QQueue<QByteArray> m_lstInbox;
    QList<QByteArray> m_lstServerReply;

    QString m_sHost;
    int m_iPort;
    ConnectionType m_iConnectionType;
    QString m_sSecret;
    QString m_sUser;
    QString m_sPassword;
    AuthMethod m_iAuthMethod;
    QByteArray m_BATimestamp;
    Error m_iLastError;

    int m_iConnectionTimeout;
    int m_iResponseTimeout;
    int m_iSendMessageTimeout;

    bool waitForResponse(bool bMultiline);
    bool sendMessage(const QByteArray &BAText);

public:
    explicit CPop3Client(QObject *pObject = 0);
    ~CPop3Client();

    QString host() const { return m_sHost; }
    void setHost(const QString &sHost) { m_sHost = sHost; }
    int port() { return m_iPort; }
    void setPort(int iPort) { m_iPort = iPort; }
    ConnectionType connectionType() { return m_iConnectionType; }
    void setConnectionType(ConnectionType iConnectionType) { m_iConnectionType = iConnectionType; }
    QString secret() const { return m_sSecret; }
    void setSecret(const QString &sSecret) { m_sSecret = sSecret; }
    QString user() const { return m_sUser; }
    void setUser(const QString &sUser) { m_sUser = sUser; }
    QString password() const { return m_sPassword; }
    void setPassword(const QString &sPassword) { m_sPassword = sPassword; }
    AuthMethod authMethod() { return m_iAuthMethod; }
    void setAuthMethod(AuthMethod iMethod) { m_iAuthMethod = iMethod; }

    int connectionTimeout() { return m_iConnectionTimeout; }
    void setConnectionTimeout(int iMsec) { m_iConnectionTimeout = iMsec; }
    int responseTimeout() { return m_iResponseTimeout; }
    void setResponseTimeout(int iMsec) { m_iResponseTimeout = iMsec; }
    int sendMessageTimeout() { return m_iSendMessageTimeout; }
    void setSendMessageTimeout(int iMsec) { m_iSendMessageTimeout = iMsec; }

    Error lastError() const { return m_iLastError; }
    bool connectToHost();
    bool disconnectFromHost();
    bool login();
    bool login(const QString &sUser, const QString &sPassword, AuthMethod iMethod);
    bool readMail();
    QByteArray nextMailFromInbox();
    QQueue<QByteArray> inbox() { return m_lstInbox; }

signals:
private slots:
};

/**************  Communication protocol ***************

CONNECTION
Client connect ->   Server reply +OK & Timespan
Client send STLS -> Server reply +OK (only for TLS connection)
Client start encryption -> Server handshake (only for TLS connection)

LOGIN
Client send APOP user MD5(timespan+secret) -> Server reply +OK (only for AuthAPOP)
Client send USER user -> Server reply +OK (only for AuthPLAIN)
Client send PASS pass -> Server reply +OK (only for AuthPLAIN)

READ MAIL
Client send STAT -> Server reply +OK n (n = number of mails)
Client send RETR i -> Server reply +OK / message number i in lines finished with .\r\n
Client send DELE i -> Server reply +OK & mark for deletion message number i

DISCONNECTION
Client send QUIT -> Server delete message from host

- Server would reply -ERR in case of an error

*******************************************************/

#endif // DR_CPOP3CLIENT_H
