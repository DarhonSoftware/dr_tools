//Release 2
#include "dr_pop3client.h"
#include <QtCore>
#include <QtNetwork>

CPop3Client::CPop3Client(QObject *pObject)
    : QObject(pObject)
{
    m_pSocket = 0;

    m_sHost.clear();
    m_iPort = 0;
    m_iConnectionType = SslConnection;
    m_sSecret.clear();
    m_sUser.clear();
    m_sPassword.clear();
    m_iAuthMethod = AuthPlain;
    m_iLastError = ErrorNoError;

    setConnectionTimeout(30000);
    setResponseTimeout(30000);
    setSendMessageTimeout(30000);
}

CPop3Client::~CPop3Client() {}

bool CPop3Client::connectToHost()
{
    //Validate if socket is already connected
    if (m_pSocket) {
        m_iLastError = ErrorSocketInUse;
        return false;
    }

    //Create & connect socket based on connection type
    switch (m_iConnectionType) {
    case TcpConnection:
        m_pSocket = new QTcpSocket(this);
        m_pSocket->connectToHost(m_sHost, m_iPort);
        break;

    case SslConnection:
        m_pSocket = new QSslSocket(this);
        ((QSslSocket *) m_pSocket)->connectToHostEncrypted(m_sHost, m_iPort);
        break;

    case TlsConnection:
        m_pSocket = new QSslSocket(this);
        m_pSocket->connectToHost(m_sHost, m_iPort);
        break;
    }

    //Connect to POP3 server
    if (!m_pSocket->waitForConnected(m_iConnectionTimeout)) {
        m_iLastError = ErrorSocketTimeoutConnection;
        return false;
    }

    //Comunication: Confirm Server is ready
    if (!waitForResponse(false))
        return false;
    if (m_lstServerReply.first().left(3) != "+OK") {
        m_iLastError = ErrorReplyConnection;
        return false;
    }

    //Obtain the timestamp from the creeting for servers supporting APOP
    int i1 = m_lstServerReply.first().indexOf("<");
    int i2 = m_lstServerReply.first().indexOf(">");
    if ((i1 != -1) && (i2 != -1) && (i1 < i2))
        m_BATimestamp = m_lstServerReply.first().mid(i1, i2 - i1 + 1);
    else
        m_BATimestamp.clear();

    //** AUTHORIZATION state **//

    //TSL handshaking protocol
    if (m_iConnectionType == TlsConnection) {
        //Communication: STSL command
        if (sendMessage("STLS"))
            return false;
        if (!waitForResponse(false))
            return false;
        if (m_lstServerReply.first().left(3) != "+OK") {
            m_iLastError = ErrorReplySTLS;
            return false;
        };

        //Initiates encryption
        ((QSslSocket *) m_pSocket)->startClientEncryption();
        if (!((QSslSocket *) m_pSocket)->waitForEncrypted(m_iConnectionTimeout)) {
            m_iLastError = ErrorSocketEncryption;
            return false;
        }
    }

    m_iLastError = ErrorNoError;
    return true;

    //** AUTHORIZATION state **//
}

bool CPop3Client::disconnectFromHost()
{
    //Validate if socket is connected
    if (!m_pSocket) {
        m_iLastError = ErrorSocketInvalid;
        return false;
    }

    //** UPDATE state **//

    //Communication: QUIT command
    sendMessage("QUIT");
    waitForResponse(false);

    //** UPDATE state **//

    //Disconnects from POP3 server
    m_pSocket->disconnectFromHost();
    if (m_pSocket->state() != QAbstractSocket::UnconnectedState)
        m_pSocket->waitForDisconnected(m_iConnectionTimeout);

    //Reset parameters after successful disconnection
    delete m_pSocket;
    m_pSocket = 0;

    m_iLastError = ErrorNoError;
    return true;
}

bool CPop3Client::login()
{
    return login(m_sUser, m_sPassword, m_iAuthMethod);
}

bool CPop3Client::login(const QString &sUser, const QString &sPassword, AuthMethod iMethod)
{
    //Validate if socket is connected
    if (!m_pSocket) {
        m_iLastError = ErrorSocketInvalid;
        return false;
    }

    //** AUTHORIZATION state **//

    if (iMethod == AuthAPOP) {
        //Communication: APOP command [username + ' ' + md5(timestamp+secret)]
        if (!sendMessage("APOP " + sUser.toUtf8() + " " + QCryptographicHash::hash(m_BATimestamp + m_sSecret.toUtf8(), QCryptographicHash::Md5)))
            return false;
        if (!waitForResponse(false))
            return false;
        if (m_lstServerReply.first().left(3) != "+OK") {
            m_iLastError = ErrorReplyAPOP;
            return false;
        }
    }

    else if (iMethod == AuthPlain) {
        //Communication: USER command
        if (!sendMessage("USER " + sUser.toUtf8()))
            return false;
        if (!waitForResponse(false))
            return false;
        if (m_lstServerReply.first().left(3) != "+OK") {
            m_iLastError = ErrorReplyUSER;
            return false;
        }

        //Communication: PASS command
        if (!sendMessage("PASS " + sPassword.toUtf8()))
            return false;
        if (!waitForResponse(false))
            return false;
        if (m_lstServerReply.first().left(3) != "+OK") {
            m_iLastError = ErrorReplyPASS;
            return false;
        }
    }

    //** TRANSACTION state **//

    m_iLastError = ErrorNoError;
    return true;
}

bool CPop3Client::readMail()
{
    //Validate if socket is connected
    if (!m_pSocket) {
        m_iLastError = ErrorSocketInvalid;
        return false;
    }

    //** TRANSACTION state **//

    //Communication: STAT command & get number of mails
    if (!sendMessage("STAT"))
        return false;
    if (!waitForResponse(false))
        return false;
    if (m_lstServerReply.first().left(3) != "+OK") {
        m_iLastError = ErrorReplySTAT;
        return false;
    }
    int iMails = m_lstServerReply.first().split(' ').at(1).toInt();

    //Communication: RETR command
    for (int i = 0; i < iMails; i++) {
        if (!sendMessage("RETR " + QByteArray::number(i + 1)))
            return false;
        if (!waitForResponse(true))
            return false;
        if (m_lstServerReply.first().left(3) != "+OK") {
            m_iLastError = ErrorReplyRETR;
            return false;
        }
        m_lstInbox.enqueue(m_lstServerReply.last().left(m_lstServerReply.last().length() - 2));
    }

    //Communication: DELE command
    for (int i = 0; i < iMails; i++) {
        if (!sendMessage("DELE " + QByteArray::number(i + 1)))
            return false;
        if (!waitForResponse(false))
            return false;
        if (m_lstServerReply.first().left(3) != "+OK") {
            m_iLastError = ErrorReplyDELE;
            return false;
        }
    }

    m_iLastError = ErrorNoError;
    return true;

    //** TRANSACTION state **//
}

QByteArray CPop3Client::nextMailFromInbox()
{
    if (m_lstInbox.count() > 0)
        return m_lstInbox.dequeue();
    else
        return QByteArray();
}

bool CPop3Client::waitForResponse(bool bMultiline)
{
    //Reset reply variable
    m_lstServerReply.clear();

    //Read all lines from buffer
    m_iLastError = ErrorNoError;
    do {
        if (!m_pSocket->waitForReadyRead(m_iResponseTimeout)) {
            m_iLastError = ErrorSocketTimeoutReadyRead;
            return false;
        }

        while (m_pSocket->canReadLine()) {
            QByteArray BAResponse = m_pSocket->readLine();
            if ((bMultiline) && (BAResponse == ".\r\n"))
                return true;
            m_lstServerReply.append(BAResponse);
            if (!bMultiline)
                return true;
            if (BAResponse.left(4) == "-ERR")
                return true;
        }
    } while (true);
}

bool CPop3Client::sendMessage(const QByteArray &BAText)
{
    m_pSocket->write(BAText + "\r\n");

    if (!m_pSocket->waitForBytesWritten(m_iSendMessageTimeout)) {
        m_iLastError = ErrorSocketTimeoutBytesWritten;
        return false;
    }

    m_iLastError = ErrorNoError;
    return true;
}
