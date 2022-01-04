#ifndef DR_CSMTPCLIENT_H
#define DR_CSMTPCLIENT_H

#include <QObject>
#include <QTcpSocket>

class CSmtpClient : public QObject
{
  Q_OBJECT

public:
  enum AuthMethod {AuthPlain,AuthLogin};
  enum ConnectionType {TcpConnection,SslConnection,TlsConnection};
  enum Error {
    ErrorNoError,                   // No error occurred
    ErrorSocketTimeoutConnection,   // Socket timeout while waiting for connection
    ErrorSocketTimeoutReadyRead,    // Socket timeout while waiting for data to read from buffer
    ErrorSocketTimeoutBytesWritten, // Socket timeout while waiting to write data to buffer
    ErrorSocketInUse,               // Socket is already connected
    ErrorSocketInvalid,             // Socket is not connected
    ErrorSocketEncryption,          // Socket failed to complete handshake for encryption
    ErrorReplyConnection,           // Server reply -ERR after connection attempt
    ErrorReplyEHLO,                 // Server reply -ERR after command EHLO
    ErrorReplySTARTTLS,             // Server reply -ERR after command STARTTLS
    ErrorReplyUSER,                 // Server reply -ERR after command USER
    ErrorReplyPASS,                 // Server reply -ERR after command PASS
    ErrorReplyAUTHPLAIN,            // Server reply -ERR after command AUTHPLAIN
    ErrorReplyAUTHLOGIN,            // Server reply -ERR after command AUTHLOGIN
    ErrorReplyFROM,                 // Server reply -ERR after command FROM
    ErrorReplyTO,                   // Server reply -ERR after command TO
    ErrorReplyDATA,                 // Server reply -ERR after command DATA
    ErrorReplyMail,                 // Server reply -ERR after sending mail
  };

private:
  QTcpSocket *m_pSocket;
  int m_iServerReply;

  QString m_sHost;
  int m_iPort;
  ConnectionType m_iConnectionType;
  QString m_sName;
  QString m_sUser;
  QString m_sPassword;
  AuthMethod m_iAuthMethod;
  Error m_iLastError;

  int m_iConnectionTimeout;
  int m_iResponseTimeout;
  int m_iSendMessageTimeout;

  bool waitForResponse();
  bool sendMessage(const QByteArray &BAText);

public:
  explicit CSmtpClient(QObject *pObject = 0);
  ~CSmtpClient();

  QString host() const {return m_sHost;}
  void setHost(const QString &sHost) {m_sHost = sHost;}
  int port() {return m_iPort;}
  void setPort(int iPort) {m_iPort = iPort;}
  ConnectionType connectionType() {return m_iConnectionType;}
  void setConnectionType(ConnectionType iConnectionType) {m_iConnectionType = iConnectionType;}
  QString name() const {return m_sName;}
  void setName(const QString &sName) {m_sName = sName;}
  QString user() const {return m_sUser;}
  void setUser(const QString &sUser) {m_sUser = sUser;}
  QString password() const {return m_sPassword;}
  void setPassword(const QString &sPassword) {m_sPassword = sPassword;}
  AuthMethod authMethod() {return m_iAuthMethod;}
  void setAuthMethod(AuthMethod iMethod) {m_iAuthMethod = iMethod;}

  int connectionTimeout() {return m_iConnectionTimeout;}
  void setConnectionTimeout(int iMsec) {m_iConnectionTimeout = iMsec;}
  int responseTimeout() {return m_iResponseTimeout;}
  void setResponseTimeout(int iMsec) {m_iResponseTimeout = iMsec;}
  int sendMessageTimeout() {return m_iSendMessageTimeout;}
  void setSendMessageTimeout(int iMsec) {m_iSendMessageTimeout = iMsec;}

  Error lastError() const {return m_iLastError;}
  bool connectToHost();
  bool disconnectFromHost();
  bool login();
  bool login(const QString &sUser, const QString &sPassword, AuthMethod iMethod);
  bool sendMail(const QString &sFrom,const QString &sNameFrom, const QString &sTo, const QString &sNameTo, const QString &sSubject, const QString &sBody);

signals:
private slots:
};

/**************  Communication protocol ***************

CONNECTION
Client connect ->   Server reply 220
Client send EHLO -> Server reply 250
Client send STARTTLS -> Server reply 220 (only for TLS connection)
Client start encryption -> Server handshake (only for TLS connection)
Client send EHLO -> Server reply 250 (only for TLS connection)

LOGIN
Client send AUTH PLAIN Base64('\0' + username + '\0' + password) -> Server reply 235 (only for AuthPLAIN)
Client send AUTH LOGIN -> Server reply 334 (only for LoginPLAIN)
Client send Base64(user) -> Server reply 334 (only for LoginPLAIN)
Client send Base64(pass) -> Server reply 235 (only for LoginPLAIN)

SEND MAIL
Client send MAIL FROM:<..> -> Server reply 250
Client send RCPT TO:<..> -> Server reply 250
Client send DATA -> Server reply 354
Client send message + .\r\n -> Server reply 250

DISCONNECTION
Client send QUIT -> Server realese resources

*******************************************************/

#endif // DR_CSMTPCLIENT_H
