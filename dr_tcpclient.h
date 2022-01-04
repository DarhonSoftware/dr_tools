#ifndef DR_CTCPCLIENT_H
#define DR_CTCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QQueue>
#include <QByteArray>
#include <QHostAddress>
#include "dr_crypt.h"

class CTcpClient : public QObject
{
  Q_OBJECT

public:
  enum Error {
    ErrorNoError,                   // No error occurred.
    ErrorNoPassSet,                 // No pass was set. You can not use encrypt/decrypt capability without password.
    ErrorNoMarkSet,                 // No mark was set. You can not build a package without a mark.
    ErrorPackageSize,               // Package size is bigger than 65k
    ErrorSocketTimeoutConnection,   // Socket timeout while waiting for connection
    ErrorSocketTimeoutReadyRead,    // Socket timeout while waiting for data to read from buffer
    ErrorSocketTimeoutBytesWritten, // Socket timeout while waiting to write data to buffer
    ErrorSocketInUse,               // Socket is already connected
    ErrorSocketPackageMark,         // Socket received has a wrong package mark
    ErrorSocketPassword,            // Socket received has a wrong password
    ErrorSocketUnconnected,         // Socket is not connected for use
    ErrorSocketAlreadyConnected     // Socket is already connected
  };

private:
  QTcpSocket *m_pTcpClient;
  QByteArray m_BAServerReply;

  CCrypt m_Crypt;
  QByteArray m_BAPackageMark;
  QString m_sPassword;
  Error m_iLastError;

  int m_iConnectionTimeout;
  int m_iResponseTimeout;
  int m_iSendMessageTimeout;

public:
  CTcpClient(QObject *pObject=0);
  ~CTcpClient();

  void setPackageMark(const QByteArray& BAPackageMark);
  QByteArray packageMark() const {return m_BAPackageMark;}
  void setPassword(const QString& sPassword);
  QString password() const {return m_sPassword;}

  int connectionTimeout() {return m_iConnectionTimeout;}
  void setConnectionTimeout(int iMsec) {m_iConnectionTimeout = iMsec;}
  int responseTimeout() {return m_iResponseTimeout;}
  void setResponseTimeout(int iMsec) {m_iResponseTimeout = iMsec;}
  int sendMessageTimeout() {return m_iSendMessageTimeout;}
  void setSendMessageTimeout(int iMsec) {m_iSendMessageTimeout = iMsec;}

  bool connectToServer(const QString &sHost, quint16 u16Port);
  bool disconnectFromServer();
  bool sendMessage(const QByteArray &BAData);
  bool waitForResponse();
  QByteArray serverReply();
  Error lastError() const {return m_iLastError;}

private:
signals:
private slots:
};

/**************  Package structure *******************

8   Package mark
2   Package size
X   Data

*******************************************************/

/**************  Communication protocol ***************

Client connect ->   Server accepts connection
Client send message -> Server process message
Server reply result -> Client process reply
Client disconnect -> Server free resources

*******************************************************/


#endif // DR_CTCPCLIENT_H
