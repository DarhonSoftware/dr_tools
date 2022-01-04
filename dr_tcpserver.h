#ifndef DR_CTCPSERVER_H
#define DR_CTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QQueue>
#include <QPair>
#include <QByteArray>
#include "dr_crypt.h"

class CTcpServer : public QObject
{
  Q_OBJECT

public:
  enum Error {
    ErrorNoError,                    // No error occurred.
    ErrorNoPassSet,                  // No pass was set. You can not use encrypt/decrypt capability without password.
    ErrorNoMarkSet,                  // No mark was set. You can not build a package without a mark.
    ErrorPackageSize,                // Package size is bigger than 65k
    ErrorSocketListen,               // Server socket cannot listen
    ErrorSocketAlreadyListening,     // Socket is already listening
    ErrorSocketWrite,                // Write operation to transport socket failed
    ErrorSocketUnconnected           // Socket is not connected for use
  };

private:
  QTcpServer *m_pTcpServer;
  QHash<QTcpSocket*,quint16> m_hashPackageSize;

  CCrypt m_Crypt;
  QByteArray m_BAPackageMark;
  QString m_sPassword;
  QHostAddress m_Host;
  quint16 m_u16Port;
  Error m_iLastError;

public:
  CTcpServer(QObject *pObject=0);
  ~CTcpServer();

  void setPackageMark(const QByteArray& BAPackageMark);
  QByteArray packageMark() const {return m_BAPackageMark;}
  void setPassword(const QString& sPassword);
  QString password() const {return m_sPassword;}
  void setHost (const QHostAddress& Host) {m_Host=Host;}
  QHostAddress host() const {return m_Host;}
  void setPort (quint16 u16Port) {m_u16Port=u16Port;}
  quint16 port() const {return m_u16Port;}

  bool startServer();
  bool stopServer();
  bool sendMessage(QTcpSocket* pSocket,const QByteArray &BAData);
  Error lastError() const {return m_iLastError;}

private:

signals:
  void messageReceived(QTcpSocket* pSocket,const QByteArray &BAData);

private slots:
  void serverNewConnection();
  void socketDisconnected();
  void socketReadyRead();
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

#endif // DR_CTCPSERVER_H
