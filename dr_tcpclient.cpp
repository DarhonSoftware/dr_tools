#include <QtCore>
#include "dr_tcpclient.h"

#define CRYPT_MARK             "e566dfe160ada16d"
#define PACKAGE_MARK_SIZE      8

CTcpClient::CTcpClient(QObject *pObject) : QObject(pObject)
{
  //Initialize parameters
  m_pTcpClient=new QTcpSocket(this);
  m_BAPackageMark.clear();
  m_sPassword.clear();
  m_iLastError=ErrorNoError;

  setConnectionTimeout(30000);
  setResponseTimeout(30000);
  setSendMessageTimeout(30000);

  //Setup cryptography
  m_Crypt.setMark(QByteArray::fromHex(CRYPT_MARK));
  m_Crypt.setCompressionMode(CCrypt::CompressionNo);
  m_Crypt.setEncryptionMode(CCrypt::SIM128);
}

CTcpClient::~CTcpClient()
{
  delete m_pTcpClient;
}

void CTcpClient::setPackageMark(const QByteArray& BAPackageMark)
{
  if (BAPackageMark.size()==PACKAGE_MARK_SIZE) m_BAPackageMark = BAPackageMark;
}

void CTcpClient::setPassword(const QString& sPassword)
{
  m_sPassword=sPassword;
  m_Crypt.setPass(m_sPassword.toUtf8());
}

bool CTcpClient::connectToServer(const QString &sHost, quint16 u16Port)
{
  //Validate if socket is disconnected
  if (m_pTcpClient->state()==QAbstractSocket::ConnectedState)
  {
    m_iLastError = ErrorSocketAlreadyConnected;
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

  //Connect socket to server
  m_pTcpClient->connectToHost(sHost,u16Port);
  if (!m_pTcpClient->waitForConnected(m_iConnectionTimeout))
  {
    m_iLastError=ErrorSocketTimeoutConnection;
    return false;
  }

  m_iLastError=ErrorNoError;
  return true;
}

bool CTcpClient::disconnectFromServer()
{
  //Validate if socket is connected
  if (m_pTcpClient->state()==QAbstractSocket::UnconnectedState)
  {
    m_iLastError = ErrorSocketUnconnected;
    return false;
  }

  m_pTcpClient->disconnectFromHost();
  if (m_pTcpClient->state() != QAbstractSocket::UnconnectedState)
    m_pTcpClient->waitForDisconnected(m_iConnectionTimeout);

  m_iLastError=ErrorNoError;
  return true;
}

bool CTcpClient::sendMessage(const QByteArray &BAData)
{
  //Validate if socket is connected
  if (m_pTcpClient->state()==QAbstractSocket::UnconnectedState)
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
  m_pTcpClient->write(ByteArray);
  if (!m_pTcpClient->waitForBytesWritten(m_iSendMessageTimeout))
  {
    m_iLastError=ErrorSocketTimeoutBytesWritten;
    return false;
  }

  m_iLastError=ErrorNoError;
  return true;
}

bool CTcpClient::waitForResponse()
{
  //Validate if socket is connected
  if (m_pTcpClient->state()==QAbstractSocket::UnconnectedState)
  {
    m_iLastError = ErrorSocketUnconnected;
    return false;
  }

  //Setup DataSream
  QDataStream DataStream(m_pTcpClient);
  DataStream.setVersion(QDataStream::Qt_5_0);
  quint16 u16PackageSize=0;
  m_BAServerReply.clear();

  do
  {
    if (!m_pTcpClient->waitForReadyRead(m_iResponseTimeout))
    {
      m_iLastError=ErrorSocketTimeoutReadyRead;
      return false;
    }

    //Read heading for new package MARK + SIZE
    if (u16PackageSize == 0)
    {
      if (m_pTcpClient->bytesAvailable() < (int)(sizeof(quint16)+PACKAGE_MARK_SIZE)) continue;

      //Drop connection if unexpected mark
      if (m_BAPackageMark!=DataStream.device()->read(PACKAGE_MARK_SIZE))
      {
        m_pTcpClient->abort();
        m_iLastError=ErrorSocketPackageMark;
        return false;
      }

      //Record package size for further processing
      DataStream >> u16PackageSize;
    }

    //Evaluate if buffer contains the whole data expected in this package
    if (m_pTcpClient->bytesAvailable() < u16PackageSize) continue;

    //Read data from package
    DataStream >> m_BAServerReply;
    m_BAServerReply=m_Crypt.decryptArray(m_BAServerReply);

    //Validate received data - TooShort/NoCryptMark/CryptVer/WrongPass/IntegrityFailed
    if (m_Crypt.lastError()!=CCrypt::ErrorNoError)
    {
      m_BAServerReply.clear();
      m_pTcpClient->abort();
      m_iLastError=ErrorSocketPassword;
      return false;
    }

    //Once the whole package is read and stored the process finish
    return true;

  } while (true);
}

QByteArray CTcpClient::serverReply()
{
  return m_BAServerReply;
}
