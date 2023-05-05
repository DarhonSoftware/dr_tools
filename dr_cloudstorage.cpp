//Release 1
#include "dr_cloudstorage.h"
#include <QDesktopServices>
#include <QRandomGenerator>
#include <QtCore>
#include <QtNetwork>
#include "dr_crypt.h"

#define CRYPT_MARK "f10a490bc629b16d"
#define CRYPT_PASS "cloudstorage"

CCloudStorage::CCloudStorage(QObject *pObject)
    : QObject(pObject)
{
    //Setup cryptography
    m_Crypt.setMark(QByteArray::fromHex(CRYPT_MARK));
    m_Crypt.setCompressionMode(CCrypt::CompressionNo);
    m_Crypt.setEncryptionMode(CCrypt::SIM128);
    m_Crypt.setPass(QString(CRYPT_PASS).toUtf8());

    //Settings - Begin group
    QSettings Settings;
    Settings.beginGroup("CloudStorage");

    //Load access tokens from file
    m_sOdRefreshToken = m_Crypt.decryptArray(
        QByteArray::fromBase64(Settings.value("od_token").toByteArray()));
    m_sDbRefreshToken = m_Crypt.decryptArray(
        QByteArray::fromBase64(Settings.value("db_token").toByteArray()));
    m_sGdRefreshToken = m_Crypt.decryptArray(
        QByteArray::fromBase64(Settings.value("gd_token").toByteArray()));

    //Load Google Drive parameters from file
    m_sGdFileId = m_Crypt.decryptArray(
        QByteArray::fromBase64(Settings.value("gd_fileId").toByteArray()));

    //Settings - End group
    Settings.endGroup();

    //Initialize members
    setRunning(false);
    setOdauth(!m_sOdRefreshToken.isEmpty());
    setDbauth(!m_sDbRefreshToken.isEmpty());
    setGdauth(!m_sGdRefreshToken.isEmpty());
    m_BACodeVerifier.clear();

    //Initialize credencials
    m_pairOdCredentials.first = QString();
    m_pairOdCredentials.second = QString();
    m_pairDbCredentials.first = QString();
    m_pairDbCredentials.second = QString();
    m_pairGdCredentials.first = QString();
    m_pairGdCredentials.second = QString();

    //Initialise RedirectURI
    m_sOdRedirectURI.clear();
    m_sDbRedirectURI.clear();
    m_sGdRedirectURI.clear();

    //Connect Network Manager
    connect(&m_Manager, &QNetworkAccessManager::finished, this, &CCloudStorage::replyFinished);
}

CCloudStorage::~CCloudStorage()
{
    //Settings - Begin group
    QSettings Settings;
    Settings.beginGroup("CloudStorage");

    //Save access tokens to file
    Settings.setValue("od_token", m_Crypt.encryptArray(m_sOdRefreshToken.toUtf8()).toBase64());
    Settings.setValue("db_token", m_Crypt.encryptArray(m_sDbRefreshToken.toUtf8()).toBase64());
    Settings.setValue("gd_token", m_Crypt.encryptArray(m_sGdRefreshToken.toUtf8()).toBase64());

    //Save Google Drive parameters to file
    Settings.setValue("gd_fileId", m_Crypt.encryptArray(m_sGdFileId.toUtf8()).toBase64());

    //Settings - End group
    Settings.endGroup();
}

QUrl CCloudStorage::odAuthorizationRequest()
{
    //Create code verifier - 47 bytes
    m_BACodeVerifier = QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier = m_BACodeVerifier.toBase64(QByteArray::Base64UrlEncoding);
    m_BACodeVerifier.truncate(m_BACodeVerifier.length() - 1);

    //Create code challenge - 43 bytes
    QByteArray BACodeChallenge = QCryptographicHash::hash(m_BACodeVerifier,
                                                          QCryptographicHash::Sha256)
                                     .toBase64(QByteArray::Base64UrlEncoding);
    BACodeChallenge.truncate(BACodeChallenge.length() - 1);

    return QUrl(QString("https://login.microsoftonline.com/common/oauth2/v2.0/authorize?"
                        "client_id=%1&"
                        "response_type=code&"
                        "redirect_uri=%2&"
                        "scope=files.readwrite offline_access&"
                        "response_mode=query&"
                        "code_challenge=%3&"
                        "code_challenge_method=S256")
                    .arg(m_pairOdCredentials.first, m_sOdRedirectURI, BACodeChallenge));
}

bool CCloudStorage::odRefreshToken(const QString &sUrl)
{
    //Read the Url and identify if final authorisation is provided
    QString sAuthorizationCode;
    QUrl Url;
    Url.setUrl(sUrl);

    if ((Url.hasQuery()) && (Url.query().startsWith("code=")))
        sAuthorizationCode = Url.query().remove("code=");
    else
        return false;

    //Set state
    m_iAction = ActionOdRefreshToken;

    //Extract code & create query
    QString s = QString("client_id=%1&"
                        "grant_type=authorization_code&"
                        "scope=files.readwrite offline_access&"
                        "code=%2&"
                        "redirect_uri=%3&"
                        "code_verifier=%4")
                    .arg(m_pairOdCredentials.first,
                         sAuthorizationCode,
                         m_sOdRedirectURI,
                         m_BACodeVerifier);

    QNetworkRequest Request;
    Request.setUrl(QUrl("https://login.microsoftonline.com/common/oauth2/v2.0/token"));
    Request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    Request.setHeader(QNetworkRequest::ContentLengthHeader, s.toUtf8().size());

    //Send request
    setRunning(true);
    m_Manager.post(Request, s.toUtf8());

    return true;
}

void CCloudStorage::odCallAPI(bool bUpload, const QString &sLocal, const QString &sRemote)
{
    //Validate if another network request is already in progress
    if (running())
        return;

    //Validate if refresh token is valid
    if (!odauth())
        return;

    //Set state
    m_iAction = ActionOdAccessToken;

    //Set parameters for Upload/Download routines
    m_sLocal = sLocal;
    m_sRemote = sRemote;
    m_bUpload = bUpload;

    //Extract code & create query
    QString s = QString("client_id=%1&"
                        "grant_type=refresh_token&"
                        "scope=files.readwrite offline_access&"
                        "refresh_token=%2")
                    .arg(m_pairOdCredentials.first, m_sOdRefreshToken);

    QNetworkRequest Request;
    Request.setUrl(QUrl("https://login.microsoftonline.com/common/oauth2/v2.0/token"));
    Request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    Request.setHeader(QNetworkRequest::ContentLengthHeader, s.toUtf8().size());

    //Send request
    setRunning(true);
    m_Manager.post(Request, s.toUtf8());
}

void CCloudStorage::odDeleteAccessToken()
{
    m_sOdRefreshToken.clear();
    setOdauth(false);
}

bool CCloudStorage::odUpload()
{
    //Validate if file exists
    if (!QFile(m_sLocal).exists()) {
        emit uploadResult(false, "OneDrive", tr("Your database has not been created as yet"));
        return false;
    }

    //Set state
    m_iAction = ActionOdUpload;

    //Open file and read all content locally
    QFile File(m_sLocal);
    File.open(QIODevice::ReadOnly);
    m_BABuffer = File.readAll();
    File.close();

    //Prepare Request
    QNetworkRequest Request;
    Request.setUrl(QUrl(
        QString("https://graph.microsoft.com/v1.0/me/drive/root:/%1:/content").arg(m_sRemote)));
    Request.setHeader(QNetworkRequest::ContentLengthHeader, m_BABuffer.size());
    Request.setRawHeader(QString("Authorization").toUtf8(),
                         QString("Bearer %1").arg(m_sOdAccessToken).toUtf8());

    //Send request
    setRunning(true);
    m_Manager.put(Request, m_BABuffer);
    return true;
}

bool CCloudStorage::odDownload()
{
    //Set state
    m_iAction = ActionOdDownload;

    //Prepare Request
    QNetworkRequest Request;
    Request.setUrl(QUrl(
        QString("https://graph.microsoft.com/v1.0/me/drive/root:/%1:/content").arg(m_sRemote)));
    Request.setRawHeader(QString("Authorization").toUtf8(),
                         QString("Bearer %1").arg(m_sOdAccessToken).toUtf8());

    //Send request
    setRunning(true);
    m_Manager.get(Request);
    return true;
}

QUrl CCloudStorage::dbAuthorizationRequest()
{
    //Create code verifier - 47 bytes
    m_BACodeVerifier = QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier = m_BACodeVerifier.toBase64(QByteArray::Base64UrlEncoding);
    m_BACodeVerifier.truncate(m_BACodeVerifier.length() - 1);

    //Create code challenge - 43 bytes
    QByteArray BACodeChallenge = QCryptographicHash::hash(m_BACodeVerifier,
                                                          QCryptographicHash::Sha256)
                                     .toBase64(QByteArray::Base64UrlEncoding);
    BACodeChallenge.truncate(BACodeChallenge.length() - 1);

    //Open browser to request user approval and code
    return QUrl(QString("https://www.dropbox.com/oauth2/authorize?"
                        "response_type=code&"
                        "client_id=%1&"
                        "redirect_uri=%2&"
                        "scope=files.content.read files.content.write&"
                        "token_access_type=offline&"
                        "code_challenge=%3&"
                        "code_challenge_method=S256")
                    .arg(m_pairDbCredentials.first, m_sDbRedirectURI, BACodeChallenge));
}

bool CCloudStorage::dbRefreshToken(const QString &sUrl)
{
    //Read the Url and identify if final authorisation is provided
    QString sAuthorizationCode;
    QUrl Url;
    Url.setUrl(sUrl);

    if ((Url.hasQuery()) && (Url.query().startsWith("code=")))
        sAuthorizationCode = Url.query().remove("code=");
    else
        return false;

    //Set state
    m_iAction = ActionDbRefreshToken;

    //Extract code & create query
    QString s = QString("code=%1&"
                        "grant_type=authorization_code&"
                        "redirect_uri=%2&"
                        "code_verifier=%3&"
                        "client_id=%4")
                    .arg(sAuthorizationCode,
                         m_sDbRedirectURI,
                         m_BACodeVerifier,
                         m_pairDbCredentials.first);

    QNetworkRequest Request;
    Request.setUrl(QUrl("https://api.dropboxapi.com/oauth2/token"));
    Request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    Request.setHeader(QNetworkRequest::ContentLengthHeader, s.toUtf8().size());

    //Send request
    setRunning(true);
    m_Manager.post(Request, s.toUtf8());

    return true;
}

void CCloudStorage::dbCallAPI(bool bUpload, const QString &sLocal, const QString &sRemote)
{
    //Validate if another network request is already in progress
    if (running())
        return;

    //Validate if refresh token is valid
    if (!dbauth())
        return;

    //Set state
    m_iAction = ActionDbAccessToken;

    //Set parameters for Upload/Download routines
    m_sLocal = sLocal;
    m_sRemote = sRemote;
    m_bUpload = bUpload;

    //Extract code & create query
    QString s = QString("grant_type=refresh_token&"
                        "refresh_token=%1&"
                        "client_id=%2")
                    .arg(m_sDbRefreshToken, m_pairDbCredentials.first);

    QNetworkRequest Request;
    Request.setUrl(QUrl("https://api.dropboxapi.com/oauth2/token"));
    Request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    Request.setHeader(QNetworkRequest::ContentLengthHeader, s.toUtf8().size());

    //Send request
    setRunning(true);
    m_Manager.post(Request, s.toUtf8());
}

void CCloudStorage::dbDeleteAccessToken()
{
    m_sDbRefreshToken.clear();
    setDbauth(false);
}

bool CCloudStorage::dbUpload()
{
    //Validate if file exists
    if (!QFile(m_sLocal).exists()) {
        emit uploadResult(false, "Dropbox", tr("Your database has not been created as yet"));
        return false;
    }

    //Set state
    m_iAction = ActionDbUpload;

    //Open file and read all content locally
    QFile File(m_sLocal);
    File.open(QIODevice::ReadOnly);
    m_BABuffer = File.readAll();
    File.close();

    //Prepare Request
    QNetworkRequest Request;
    Request.setUrl(QUrl("https://content.dropboxapi.com/2/files/upload"));
    Request.setHeader(QNetworkRequest::ContentLengthHeader, m_BABuffer.size());
    Request.setRawHeader(QString("Authorization").toUtf8(),
                         QString("Bearer %1").arg(m_sDbAccessToken).toUtf8());
    Request.setRawHeader(QString("Dropbox-API-Arg").toUtf8(),
                         QString("{ \"path\": \"%1\",\"mode\":\"overwrite\" }")
                             .arg(m_sRemote)
                             .toUtf8());
    Request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

    //Send request
    setRunning(true);
    m_Manager.post(Request, m_BABuffer);
    return true;
}

bool CCloudStorage::dbDownload()
{
    //Set state
    m_iAction = ActionDbDownload;

    //Create local directory if doesn't exist
    QFileInfo FileInfo(m_sLocal);
    if (!FileInfo.absoluteDir().exists()) {
        QDir().mkpath(FileInfo.absolutePath());
    }

    //Prepare Request
    QNetworkRequest Request;
    Request.setUrl(QUrl("https://content.dropboxapi.com/2/files/download"));
    Request.setRawHeader(QString("Authorization").toUtf8(),
                         QString("Bearer %1").arg(m_sDbAccessToken).toUtf8());
    Request.setRawHeader(QString("Dropbox-API-Arg").toUtf8(),
                         QString("{ \"path\": \"%1\" }").arg(m_sRemote).toUtf8());

    //Send request
    setRunning(true);
    m_Manager.get(Request);
    return true;
}

QUrl CCloudStorage::gdAuthorizationRequest()
{
    //Create code verifier - 47 bytes
    m_BACodeVerifier = QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000, 99999)));
    m_BACodeVerifier = m_BACodeVerifier.toBase64(QByteArray::Base64UrlEncoding);
    m_BACodeVerifier.truncate(m_BACodeVerifier.length() - 1);

    //Create code challenge - 43 bytes
    QByteArray BACodeChallenge = QCryptographicHash::hash(m_BACodeVerifier,
                                                          QCryptographicHash::Sha256)
                                     .toBase64(QByteArray::Base64UrlEncoding);
    BACodeChallenge.truncate(BACodeChallenge.length() - 1);

    //Open browser to request user approval and code
    return QUrl(QString("https://accounts.google.com/o/oauth2/v2/auth?"
                        "client_id=%1&"
                        "redirect_uri=%2&"
                        "response_type=code&"
                        "scope=https://www.googleapis.com/auth/drive.file&"
                        "code_challenge=%3&"
                        "code_challenge_method=S256")
                    .arg(m_pairGdCredentials.first, m_sGdRedirectURI, BACodeChallenge));
}

bool CCloudStorage::gdRefreshToken(const QString &sUrl)
{
    //Read the Url and identify if final authorisation is provided
    QString sAuthorizationCode;
    QUrl Url;
    Url.setUrl(sUrl);

    if ((Url.hasQuery()) && (Url.query().startsWith("code=")))
        sAuthorizationCode = Url.query().remove("code=");
    else
        return false;

    //Set state
    m_iAction = ActionGdRefreshToken;

    //Create query
    QString s = QString("client_id=%1&"
                        "client_secret=%2&"
                        "code=%3&"
                        "code_verifier=%4&"
                        "grant_type=authorization_code&"
                        "redirect_uri=%5")
                    .arg(m_pairGdCredentials.first,
                         m_pairGdCredentials.second,
                         sAuthorizationCode,
                         m_BACodeVerifier,
                         m_sGdRedirectURI);

    QNetworkRequest Request;
    Request.setUrl(QUrl("https://oauth2.googleapis.com/token"));
    Request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    Request.setHeader(QNetworkRequest::ContentLengthHeader, s.toUtf8().size());

    //Send request
    setRunning(true);
    m_Manager.post(Request, s.toUtf8());

    return true;
}

void CCloudStorage::gdCallAPI(bool bUpload, const QString &sLocal, const QString &sRemote)
{
    //Validate if another network request is already in progress
    if (running())
        return;

    //Validate if refresh token is valid
    if (!gdauth())
        return;

    //Set state
    m_iAction = ActionGdAccessToken;

    //Set parameters for Upload/Download routines
    m_sLocal = sLocal;
    m_sRemote = sRemote;
    m_bUpload = bUpload;

    //Extract code & create query
    QString s = QString("client_id=%1&"
                        "client_secret=%2&"
                        "grant_type=refresh_token&"
                        "refresh_token=%3")
                    .arg(m_pairGdCredentials.first, m_pairGdCredentials.second, m_sGdRefreshToken);

    QNetworkRequest Request;
    Request.setUrl(QUrl("https://oauth2.googleapis.com/token"));
    Request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    Request.setHeader(QNetworkRequest::ContentLengthHeader, s.toUtf8().size());

    //Send request
    setRunning(true);
    m_Manager.post(Request, s.toUtf8());
}

void CCloudStorage::gdDeleteAccessToken()
{
    m_sGdRefreshToken.clear();
    setGdauth(false);
}

bool CCloudStorage::gdUpload()
{
    //Validate if file exists
    if (!QFile(m_sLocal).exists()) {
        emit uploadResult(false, "Google Drive", tr("Your database has not been created as yet"));
        return false;
    }

    //Set state
    m_iAction = ActionGdUpload;

    //Declare variables for Upload or Update
    QFile File(m_sLocal);
    QString sBoundary = "!1@2#3$4%5&6*7(8)9";

    m_BABuffer = QString("--%1\r\nContent-Type: application/json\r\n\r\n{\r\n'name': "
                         "'%2'\r\n}\r\n--%1\r\nContent-Type: application/octet-stream\r\n\r\n")
                     .arg(sBoundary, m_sRemote)
                     .toUtf8();

    //Open file and read all content locally
    File.open(QIODevice::ReadOnly);
    m_BABuffer.append(File.readAll());
    File.close();

    m_BABuffer.append(QString("\r\n--%1--").arg(sBoundary).toUtf8());

    //Prepare Request
    QNetworkRequest Request;
    Request.setUrl(
        QUrl(QString("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart")));
    Request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QString("multipart/related; boundary=%1").arg(sBoundary));
    Request.setHeader(QNetworkRequest::ContentLengthHeader, m_BABuffer.size());
    Request.setRawHeader(QString("Authorization").toUtf8(),
                         QString("Bearer %1").arg(m_sGdAccessToken).toUtf8());

    //Send request
    setRunning(true);
    m_Manager.post(Request, m_BABuffer);
    return true;
}

bool CCloudStorage::gdDownload()
{
    //Validate if fileId is available
    if (m_sGdFileId.isEmpty()) {
        emit downloadResult(false, "Google Drive", tr("Your database has not been uploaded as yet"));
        return false;
    }

    //Set state
    m_iAction = ActionGdDownload;

    //Create local directory if doesn't exist
    QFileInfo FileInfo(m_sLocal);
    if (!FileInfo.absoluteDir().exists()) {
        QDir().mkpath(FileInfo.absolutePath());
    }

    //Prepare Request
    QNetworkRequest Request;
    Request.setUrl(
        QUrl(QString("https://www.googleapis.com/drive/v3/files/%1?alt=media").arg(m_sGdFileId)));
    Request.setRawHeader(QString("Authorization").toUtf8(),
                         QString("Bearer %1").arg(m_sGdAccessToken).toUtf8());

    //Send request
    setRunning(true);
    m_Manager.get(Request);
    return true;
}

bool CCloudStorage::gdDelete(const QString &sId)
{
    if (sId.isEmpty())
        return false;

    //Set state
    m_iAction = ActionGdDelete;

    //Prepare Request
    QNetworkRequest Request;
    Request.setUrl(QUrl("https://www.googleapis.com/drive/v3/files/" + sId));
    Request.setRawHeader(QString("Authorization").toUtf8(),
                         QString("Bearer %1").arg(m_sGdAccessToken).toUtf8());

    //Send request
    setRunning(true);
    m_Manager.deleteResource(Request);
    return true;
}

void CCloudStorage::replyFinished(QNetworkReply *pReply)
{
    QByteArray BABody = pReply->readAll();
    setRunning(false);

    switch (m_iAction) {
    //** ONEDRIVE - Refresh token
    case ActionOdRefreshToken: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if (JsonObject.contains("refresh_token")) {
            m_sOdRefreshToken = JsonObject.value("refresh_token").toString();
            m_BACodeVerifier.clear();
            setOdauth(true);
        } else {
            m_sOdRefreshToken.clear();
            setOdauth(false);
            emit accessError("OneDrive", JsonObject.value("error_description").toString());
        }
        break;
    }

        //** ONEDRIVE - Access token
    case ActionOdAccessToken: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if (JsonObject.contains("access_token")) {
            m_sOdAccessToken = JsonObject.value("access_token").toString();
            if (m_bUpload)
                odUpload();
            else
                odDownload();
        } else {
            emit accessError("OneDrive", JsonObject.value("error_description").toString());
        }
        break;
    }

        //** ONEDRIVE - Upload
    case ActionOdUpload: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if ((JsonObject.isEmpty()) || (JsonObject.contains("error"))) {
            emit uploadResult(false,
                              "OneDrive",
                              JsonObject.value("error").toObject().value("message").toString());
        } else {
            emit uploadResult(true, "OneDrive");
        }
        break;
    }

        //** ONEDRIVE - Download
    case ActionOdDownload: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if ((BABody.isEmpty()) || (JsonObject.contains("error"))) {
            emit downloadResult(false,
                                "OneDrive",
                                JsonObject.value("error").toObject().value("message").toString());
        } else {
            QFile File(m_sLocal);
            File.open(QIODevice::WriteOnly);
            File.write(BABody);
            File.close();
            emit downloadResult(true, "OneDrive");
        }
        break;
    }

        //** DROPBOX - Refresh token
    case ActionDbRefreshToken: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if (JsonObject.contains("refresh_token")) {
            m_sDbRefreshToken = JsonObject.value("refresh_token").toString();
            m_BACodeVerifier.clear();
            setDbauth(true);
        } else {
            m_sDbRefreshToken.clear();
            setDbauth(false);
            emit accessError("Dropbox", JsonObject.value("error_description").toString());
        }
        break;
    }

        //** DROPBOX - Access token
    case ActionDbAccessToken: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if (JsonObject.contains("access_token")) {
            m_sDbAccessToken = JsonObject.value("access_token").toString();
            if (m_bUpload)
                dbUpload();
            else
                dbDownload();
        } else {
            emit accessError("Dropbox", JsonObject.value("error_description").toString());
        }
        break;
    }

        //** DROPBOX - Upload
    case ActionDbUpload: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if ((JsonObject.isEmpty()) || (JsonObject.contains("error"))) {
            emit uploadResult(false, "Dropbox", JsonObject.value("error_summary").toString());
        } else {
            emit uploadResult(true, "Dropbox");
        }
        break;
    }

        //** DROPBOX - Download
    case ActionDbDownload: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if ((BABody.isEmpty()) || (JsonObject.contains("error"))) {
            emit downloadResult(false, "Dropbox", JsonObject.value("error_summary").toString());
        } else {
            QFile File(m_sLocal);
            File.open(QIODevice::WriteOnly);
            File.write(BABody);
            File.close();
            emit downloadResult(true, "Dropbox");
        }
        break;
    }

        //** GOOGLE DRIVE - Refresh token
    case ActionGdRefreshToken: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if (JsonObject.contains("refresh_token")) {
            m_sGdRefreshToken = JsonObject.value("refresh_token").toString();
            m_BACodeVerifier.clear();
            setGdauth(true);
        } else {
            m_sGdRefreshToken.clear();
            setGdauth(false);
            emit accessError("Google Drive", JsonObject.value("error_description").toString());
        }
        break;
    }

        //** GOOGLE DRIVE - Access token
    case ActionGdAccessToken: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if (JsonObject.contains("access_token")) {
            m_sGdAccessToken = JsonObject.value("access_token").toString();
            if (m_bUpload)
                gdUpload();
            else
                gdDownload();
        } else {
            emit accessError("Google Drive", JsonObject.value("error_description").toString());
        }
        break;
    }

        //** GOOGLE DRIVE - Upload
    case ActionGdUpload: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if ((JsonObject.isEmpty()) || (JsonObject.contains("error"))) {
            emit uploadResult(false,
                              "Google Drive",
                              JsonObject.value("error").toObject().value("message").toString());
        } else {
            //Delete previous file
            gdDelete(m_sGdFileId);

            //Update parameters for new file
            m_sGdFileId = JsonObject.value("id").toString();
            emit uploadResult(true, "Google Drive");
        }
        break;
    }

        //** GOOGLE DRIVE - Download
    case ActionGdDownload: {
        //Validate operation
        QJsonDocument JsonDocument = QJsonDocument::fromJson(BABody);
        QJsonObject JsonObject = JsonDocument.object();
        if ((BABody.isEmpty()) || (JsonObject.contains("error"))) {
            emit downloadResult(false,
                                "Google Drive",
                                JsonObject.value("error").toObject().value("message").toString());
        } else {
            QFile File(m_sLocal);
            File.open(QIODevice::WriteOnly);
            File.write(BABody);
            File.close();
            emit downloadResult(true, "Google Drive");
        }
        break;
    }

        //** GOOGLE DRIVE - Delete
    case ActionGdDelete: {
        //No action required
    }
    }

    pReply->deleteLater();
}
