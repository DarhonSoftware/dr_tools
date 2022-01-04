#include <QtCore>
#include <QtNetwork>
#include "dr_smtpclient.h"

CSmtpClient::CSmtpClient(QObject *pObject) : QObject(pObject)
{
  m_pSocket=0;

  m_sHost.clear();
  m_iPort=0;
  m_iConnectionType=SslConnection;
  m_sName.clear();
  m_sUser.clear();
  m_sPassword.clear();
  m_iAuthMethod=AuthPlain;
  m_iLastError=ErrorNoError;

  setConnectionTimeout(30000);
  setResponseTimeout(30000);
  setSendMessageTimeout(30000);
}

CSmtpClient::~CSmtpClient()
{
}

bool CSmtpClient::connectToHost()
{
  //Validate if socket is already connected
  if (m_pSocket)
  {
    m_iLastError=ErrorSocketInUse;
    return false;
  }

  //Create & connect socket based on connection type
  switch (m_iConnectionType)
  {
    case TcpConnection:
      m_pSocket = new QTcpSocket(this);
      m_pSocket->connectToHost(m_sHost, m_iPort);
      break;

    case SslConnection:
      m_pSocket = new QSslSocket(this);
      ((QSslSocket*) m_pSocket)->connectToHostEncrypted(m_sHost, m_iPort);
      break;

    case TlsConnection:
      m_pSocket = new QSslSocket(this);
      m_pSocket->connectToHost(m_sHost, m_iPort);
      break;
  }

  //Connects to SMTP server
  if (!m_pSocket->waitForConnected(m_iConnectionTimeout))
  {
    m_iLastError=ErrorSocketTimeoutConnection;
    return false;
  }

  //Comunication: Confirm Server is ready
  if (!waitForResponse()) return false;
  if (m_iServerReply != 220)
  {
    m_iLastError=ErrorReplyConnection;
    return false;
  }

  //Comunication: EHLO/HELO command
  if (!sendMessage("EHLO " + m_sName.toUtf8())) return false;
  if (!waitForResponse()) return false;
  if (m_iServerReply != 250)
  {
    m_iLastError=ErrorReplyEHLO;
    return false;
  }

  //TSL handshaking protocol
  if (m_iConnectionType == TlsConnection)
  {
    //Communication: STARTTSL command
    if (sendMessage("STARTTLS")) return false;
    if (!waitForResponse()) return false;
    if (m_iServerReply != 220)
    {
      m_iLastError=ErrorReplySTARTTLS;
      return false;
    };

    //Initiates encryption
    ((QSslSocket*) m_pSocket)->startClientEncryption();
    if (!((QSslSocket*) m_pSocket)->waitForEncrypted(m_iConnectionTimeout))
    {
      m_iLastError=ErrorSocketEncryption;
      return false;
    }

    //Communication: ELHO command
    if (!sendMessage("EHLO " + m_sName.toUtf8())) return false;
    if (!waitForResponse()) return false;
    if (m_iServerReply != 250)
    {
      m_iLastError=ErrorReplyEHLO;
      return false;
    }
  }

  m_iLastError=ErrorNoError;
  return true;
}

bool CSmtpClient::disconnectFromHost()
{
  //Validate if socket is connected
  if (!m_pSocket)
  {
    m_iLastError=ErrorSocketInvalid;
    return false;
  }

  //Communication: QUIT command
  sendMessage("QUIT");
  waitForResponse();

  //Disconnects from SMTP server
  m_pSocket->disconnectFromHost();
  if (m_pSocket->state() != QAbstractSocket::UnconnectedState)
    m_pSocket->waitForDisconnected(m_iConnectionTimeout);

  //Reset parameters after successful disconnection
  delete m_pSocket;
  m_pSocket=0;

  m_iLastError=ErrorNoError;
  return true;
}

bool CSmtpClient::login()
{
  return login(m_sUser, m_sPassword, m_iAuthMethod);
}

bool CSmtpClient::login(const QString &sUser, const QString &sPassword, AuthMethod iMethod)
{
  //Validate if socket is connected
  if (!m_pSocket)
  {
    m_iLastError=ErrorSocketInvalid;
    return false;
  }

  if (iMethod == AuthPlain)
  {
    //Communication: AUTH PLAIN command ['\0' + username + '\0' + password]
    if (!sendMessage("AUTH PLAIN " + QByteArray().append(char(0)).append(sUser.toUtf8()).
                     append(char(0)).append(sPassword.toUtf8()).toBase64())) return false;
    if (!waitForResponse()) return false;
    if (m_iServerReply != 235)
    {
      m_iLastError=ErrorReplyAUTHPLAIN;
      return false;
    }
  }

  else if (iMethod == AuthLogin)
  {
    //Communication: AUTH LOGIN command
    if (!sendMessage("AUTH LOGIN")) return false;
    if (!waitForResponse()) return false;
    if (m_iServerReply != 334)
    {
      m_iLastError=ErrorReplyAUTHLOGIN;
      return false;
    }

    //Communication: Send User
    if (!sendMessage(sUser.toUtf8().toBase64())) return false;
    if (!waitForResponse()) return false;
    if (m_iServerReply != 334)
    {
      m_iLastError=ErrorReplyUSER;
      return false;
    }

    //Communication: Send Password
    if (!sendMessage(sPassword.toUtf8().toBase64())) return false;
    if (!waitForResponse()) return false;
    if (m_iServerReply != 235)
    {
      m_iLastError=ErrorReplyPASS;
      return false;
    }
  }

  m_iLastError=ErrorNoError;
  return true;
}

bool CSmtpClient::sendMail(const QString &sFrom, const QString &sNameFrom, const QString &sTo, const QString &sNameTo, const QString &sSubject, const QString &sBody)
{
  //Validate if socket is connected
  if (!m_pSocket)
  {
    m_iLastError=ErrorSocketInvalid;
    return false;
  }

  //Communication: MAIL command
  if (!sendMessage("MAIL FROM:<" + sFrom.toUtf8() + ">")) return false;
  if (!waitForResponse()) return false;
  if (m_iServerReply != 250)
  {
    m_iLastError=ErrorReplyFROM;
    return false;
  }

  //Communication: RCPT command
  if (!sendMessage("RCPT TO:<" + sTo.toUtf8() + ">")) return false;
  if (!waitForResponse()) return false;
  if (m_iServerReply != 250)
  {
    m_iLastError=ErrorReplyTO;
    return false;
  }

  //Communication: DATA command
  if (!sendMessage("DATA")) return false;
  if (!waitForResponse()) return false;
  if (m_iServerReply != 354)
  {
    m_iLastError=ErrorReplyDATA;
    return false;
  }

  //Communication: Send Body
  QByteArray BA=QByteArray("From: ").append(sNameFrom.toUtf8()).append(" <").append(sFrom).append(">\r\n").
                append("To: ").append(sNameTo.toUtf8()).append(" <").append(sTo).append(">\r\n").
                append("Subject: ").append(sSubject).append("\r\n").
                append("MIME-Version: 1.0").append("\r\n").
                append("Content-Type: text/plain; charset=us-ascii").append("\r\n").
                append("Content-Transfer-Encoding: 7bit").append("\r\n\r\n").
                append(sBody.toUtf8());
  if (!sendMessage(BA)) return false;
  if (!sendMessage(".")) return false;
  if (!waitForResponse()) return false;
  if (m_iServerReply != 250)
  {
    m_iLastError=ErrorReplyMail;
    return false;
  }

  m_iLastError=ErrorNoError;
  return true;
}

bool CSmtpClient::waitForResponse()
{
  //Reset reply variable
  m_iServerReply=0;

  //Read all lines from buffer
  m_iLastError=ErrorNoError;
  do
  {
    if (!m_pSocket->waitForReadyRead(m_iResponseTimeout))
    {
      m_iLastError=ErrorSocketTimeoutReadyRead;
      return false;
    }

    while (m_pSocket->canReadLine())
    {
      QByteArray BAResponse = m_pSocket->readLine();
      m_iServerReply=BAResponse.left(3).toInt();
      if (BAResponse.at(3)==' ') return true;
    }
  } while (true);
}

bool CSmtpClient::sendMessage(const QByteArray &BAText)
{
  m_pSocket->write(BAText + "\r\n");

  if (! m_pSocket->waitForBytesWritten(m_iSendMessageTimeout))
  {
    m_iLastError=ErrorSocketTimeoutBytesWritten;
    return false;
  }

  m_iLastError=ErrorNoError;
  return true;
}
