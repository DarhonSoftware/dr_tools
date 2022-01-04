#include <QtCore>
#include "dr_tcpserver.h"

#define CRYPT_MARK             "e566dfe160ada16d"
#define PACKAGE_MARK_SIZE      8

CTcpServer::CTcpServer(QObject *pObject) : QObject(pObject)
{
  //Initialize parameters
  m_pTcpServer=new QTcpServer(this);
  m_BAPackageMark.clear();
  m_sPassword.clear();
  m_Host=QHostAddress::Any;
  m_u16Port=0;
  m_iLastError=ErrorNoError;

  //Setup cryptography
  m_Crypt.setMark(QByteArray::fromHex(CRYPT_MARK));
  m_Crypt.setCompressionMode(CCrypt::CompressionNo);
  m_Crypt.setEncryptionMode(CCrypt::SIM128);

  //Create connections
  connect(m_pTcpServer,&QTcpServer::newConnection,this,&CTcpServer::serverNewConnection);
}

CTcpServer::~CTcpServer()
{
  delete m_pTcpServer;
}

void CTcpServer::setPackageMark(const QByteArray& BAPackageMark)
{
  if (BAPackageMark.size()==PACKAGE_MARK_SIZE) m_BAPackageMark = BAPackageMark;
}

void CTcpServer::setPassword(const QString& sPassword)
{
  m_sPassword=sPassword;
  m_Crypt.setPass(m_sPassword.toUtf8());
}

bool CTcpServer::startServer()
{
  //Validate socket if already listening
  if (m_pTcpServer->isListening())
  {
    m_iLastError = ErrorSocketAlreadyListening;
    return false;
  }

  //Validate Mark
  if (m_BAPackageMark.isEmpty())
  {
    m_iLastError = ErrorNoMarkSet;
    return false;
  }

  //Validate Pass
  if (m_sPassword.isEmpty())
  {
    m_iLastError = ErrorNoPassSet;
    return false;
  }

  //Start listen for connections
  if (!m_pTcpServer->listen(m_Host,m_u16Port))
  {
    m_iLastError=ErrorSocketListen;
    return false;
  }

  m_iLastError=ErrorNoError;
  return true;
}

bool CTcpServer::stopServer()
{
  m_pTcpServer->close();
  m_iLastError=ErrorNoError;
  return true;
}

bool CTcpServer::sendMessage(QTcpSocket* pSocket,const QByteArray &BAData)
{
  //Validate if socket is connected
  if (pSocket->state()==QAbstractSocket::UnconnectedState)
  {
    m_iLastError = ErrorSocketUnconnected;
    return false;
  }

  //Build the package
  QByteArray ByteArray;
  QDataStream DataStream(&ByteArray, QIODevice::WriteOnly);
  DataStream.setVersion(QDataStream::Qt_5_0);
  DataStream.writeRawData(m_BAPackageMark.data(),PACKAGE_MARK_SIZE);
  DataStream << (quint16)0;
  DataStream << m_Crypt.encryptArray(BAData);

  //Validate package size
  if (ByteArray.size()>65535)
  {
    m_iLastError=ErrorPackageSize;
    return false;
  }

  //Sets the package size
  DataStream.device()->seek(PACKAGE_MARK_SIZE);
  DataStream << (quint16)(ByteArray.size() - sizeof(quint16) - PACKAGE_MARK_SIZE);

  //Writes package to the socket
  if (pSocket->write(ByteArray)==-1)
  {
    m_iLastError=ErrorSocketWrite;
    return false;
  }

  m_iLastError=ErrorNoError;
  return true;
}

//****************** SERVER SLOTS *************************//

void CTcpServer::serverNewConnection()
{
  QTcpSocket *pSocket=m_pTcpServer->nextPendingConnection();

  connect(pSocket,&QTcpSocket::disconnected,this,&CTcpServer::socketDisconnected);
  connect(pSocket,&QTcpSocket::readyRead,this,&CTcpServer::socketReadyRead);
}

//****************** SOCKET SLOTS *************************//

void CTcpServer::socketDisconnected()
{
  sender()->deleteLater();
}

void CTcpServer::socketReadyRead()
{
  //Setup DataSream
  QTcpSocket *pSocket=qobject_cast<QTcpSocket*>(sender());
  QDataStream DataStream(pSocket);
  DataStream.setVersion(QDataStream::Qt_5_0);

  do
  {
    //Read heading for new package MARK + SIZE
    if (m_hashPackageSize.value(pSocket,0) == 0)
    {
      if (pSocket->bytesAvailable() < (int)(sizeof(quint16)+PACKAGE_MARK_SIZE)) return;

      //Drop connection if unexpected mark
      if (m_BAPackageMark!=DataStream.device()->read(PACKAGE_MARK_SIZE))
      {
        pSocket->abort();
        return;
      }

      //Record package size for further processing
      quint16 u16Size;
      DataStream >> u16Size;
      m_hashPackageSize.insert(pSocket,u16Size);
    }

    //Evaluate if buffer contains the whole data expected in this package
    if (pSocket->bytesAvailable() < m_hashPackageSize.value(pSocket)) return;

    //Read data from package
    QByteArray BAData;
    DataStream >> BAData;
    BAData=m_Crypt.decryptArray(BAData);

    //Validate received data - TooShort/NoCryptMark/CryptVer/WrongPass/IntegrityFailed
    if (m_Crypt.lastError()!=CCrypt::ErrorNoError)
    {
      m_hashPackageSize.remove(pSocket);
      pSocket->abort();
      return;
    }

    //Send signal for processing
    m_hashPackageSize.remove(pSocket);
    emit messageReceived(pSocket,BAData);

    //Evaluate if another package is waiting to be processed in the buffer
  } while (pSocket->bytesAvailable());
}
