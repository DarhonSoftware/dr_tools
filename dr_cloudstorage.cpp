#include <QtCore>
#include <QtNetwork>
#include <QRandomGenerator>
#include <QDesktopServices>
#include "dr_cloudstorage.h"
#include "dr_crypt.h"

#define CRYPT_MARK     "f10a490bc629b16d"
#define CRYPT_PASS     "cloudstorage"

CCloudStorage::CCloudStorage(QObject *pObject) :  QObject(pObject)
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
  m_sOdRefreshToken=m_Crypt.decryptArray(QByteArray::fromBase64(Settings.value("od_token").toByteArray()));
  m_sDbAccessToken=m_Crypt.decryptArray(QByteArray::fromBase64(Settings.value("db_token").toByteArray()));
  m_sGdRefreshToken=m_Crypt.decryptArray(QByteArray::fromBase64(Settings.value("gd_token").toByteArray()));

  //Load Google Drive parameters from file
  m_sGdFileId=m_Crypt.decryptArray(QByteArray::fromBase64(Settings.value("gd_fileId").toByteArray()));

  //Settings - End group
  Settings.endGroup();

  //Initialize members
  setRunning(false);
  setOdauth(!m_sOdRefreshToken.isEmpty());
  setDbauth(!m_sDbAccessToken.isEmpty());
  setGdauth(!m_sGdRefreshToken.isEmpty());
  m_BACodeVerifier.clear();

  //Initialize credencials
  m_pairOdCredentials.first=QString();
  m_pairOdCredentials.second=QString();
  m_pairDbCredentials.first=QString();
  m_pairDbCredentials.second=QString();
  m_pairGdCredentials.first=QString();
  m_pairGdCredentials.second=QString();

  //Connect Network Manager
  connect(&m_Manager, &QNetworkAccessManager::finished,this, &CCloudStorage::replyFinished);
}

CCloudStorage::~CCloudStorage()
{
  //Settings - Begin group
  QSettings Settings;
  Settings.beginGroup("CloudStorage");

  //Save access tokens to file
  Settings.setValue("od_token", m_Crypt.encryptArray(m_sOdRefreshToken.toUtf8()).toBase64());
  Settings.setValue("db_token", m_Crypt.encryptArray(m_sDbAccessToken.toUtf8()).toBase64());
  Settings.setValue("gd_token", m_Crypt.encryptArray(m_sGdRefreshToken.toUtf8()).toBase64());

  //Save Google Drive parameters to file
  Settings.setValue("gd_fileId", m_Crypt.encryptArray(m_sGdFileId.toUtf8()).toBase64());

  //Settings - End group
  Settings.endGroup();
}

void CCloudStorage::odAuthorizationRequest()
{
  //Open browser to request user approval and code
  if (!QDesktopServices::openUrl(QUrl(QString("https://login.microsoftonline.com/consumers/oauth2/v2.0/authorize?"\
                                              "client_id=%1&"\
                                              "response_type=code&"\
                                              "redirect_uri=%2&"\
                                              "scope=files.readwrite offline_access&"\
                                              "response_mode=query").
                                 arg(m_pairOdCredentials.first,m_sOdRedirectURI))))
    emit accessError("OneDrive",tr("There was a problem opening the browser.\nEnsure there is a default browser in your system."));
}

void CCloudStorage::odRefreshToken(const QString &sAuthorizationCode)
{
  //Validate if another network request is already in progress
  if (running()) return;

  //Validate parameter and global methods
  if (sAuthorizationCode.isEmpty()) return;

  //Set state
  m_iAction=ActionOdRefreshToken;

  //Extract code & create query
  QString s=QString("client_id=%1&"\
                    "grant_type=authorization_code&"\
                    "scope=files.readwrite offline_access&"\
                    "code=%2&"\
                    "redirect_uri=%3").
            arg(m_pairOdCredentials.first,sAuthorizationCode,m_sOdRedirectURI);

  QNetworkRequest Request;
  Request.setUrl(QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/token"));
  Request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  Request.setHeader(QNetworkRequest::ContentLengthHeader,s.toUtf8().size());

  //Send request
  m_Manager.post(Request,s.toUtf8());
  setRunning(true);
}

void CCloudStorage::odCallAPI(bool bUpload, const QString& sLocal, const QString& sRemote)
{
  //Validate if another network request is already in progress
  if (running()) return;

  //Validate if refresh token is valid
  if (!odauth()) return;

  //Set state
  m_iAction=ActionOdAccessToken;

  //Set parameters for Upload/Download routines
  m_sLocal=sLocal;
  m_sRemote=sRemote;
  m_bUpload=bUpload;

  //Extract code & create query
  QString s=QString("client_id=%1&"\
                    "grant_type=refresh_token&"\
                    "scope=files.readwrite offline_access&"\
                    "refresh_token=%2&"\
                    "redirect_uri=%3").
            arg(m_pairOdCredentials.first,m_sOdRefreshToken,m_sOdRedirectURI);

  QNetworkRequest Request;
  Request.setUrl(QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/token"));
  Request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  Request.setHeader(QNetworkRequest::ContentLengthHeader,s.toUtf8().size());

  //Send request
  m_Manager.post(Request,s.toUtf8());
  setRunning(true);

  return;
}

void CCloudStorage::odDeleteAccessToken()
{
  m_sOdRefreshToken.clear();
  setOdauth(false);
}

bool CCloudStorage::odUpload()
{
  //Validate if file exists
  if (!QFile(m_sLocal).exists())
  {
    emit uploadResult(false,"OneDrive",tr("Your database has not been created as yet"));
    return false;
  }

  //Set state
  m_iAction=ActionOdUpload;

  //Open file and read all content locally
  QFile File(m_sLocal);
  File.open(QIODevice::ReadOnly);
  m_BABuffer=File.readAll();
  File.close();

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl(QString("https://graph.microsoft.com/v1.0/me/drive/root:/%1:/content").arg(m_sRemote)));
  Request.setHeader(QNetworkRequest::ContentLengthHeader,m_BABuffer.size());
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sOdAccessToken).toUtf8());

  //Send request
  m_Manager.put(Request,m_BABuffer);

  return true;
}

bool CCloudStorage::odDownloadLink()
{
  //Set state
  m_iAction=ActionOdDownloadLink;

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl(QString("https://graph.microsoft.com/v1.0/me/drive/root:/%1:/content").arg(m_sRemote)));
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sOdAccessToken).toUtf8());

  //Send request
  m_Manager.get(Request);

  return true;
}

bool CCloudStorage::odDownloadContent(const QString &sDownloadUrl)
{
  //Set state
  m_iAction=ActionOdDownloadContent;

  //Create local directory if doesn't exist
  QFileInfo FileInfo(m_sLocal);
  if (!FileInfo.absoluteDir().exists())
  {
    QDir().mkpath(FileInfo.absolutePath());
  }

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl(sDownloadUrl));

  //Send request
  m_Manager.get(Request);

  return true;
}

void CCloudStorage::dbAuthorizationRequest()
{
  //Create code verifier - 47 bytes
  m_BACodeVerifier=QByteArray::number(QRandomGenerator::global()->bounded(10000,99999));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier=m_BACodeVerifier.toBase64(QByteArray::Base64UrlEncoding);
  m_BACodeVerifier.truncate(m_BACodeVerifier.length()-1);

  //Create code challenge - 43 bytes
  QByteArray BACodeChallenge=QCryptographicHash::hash(m_BACodeVerifier,QCryptographicHash::Sha256).toBase64(QByteArray::Base64UrlEncoding);
  BACodeChallenge.truncate(BACodeChallenge.length()-1);

  //Open browser to request user approval and code
  if (!QDesktopServices::openUrl(QUrl(QString("https://www.dropbox.com/oauth2/authorize?"\
                                              "client_id=%1&"\
                                              "response_type=code&"\
                                              "scope=files.content.read files.content.write&"\
                                              "code_challenge=%2&"\
                                              "code_challenge_method=S256").
                                 arg(m_pairDbCredentials.first,BACodeChallenge))))
    emit accessError("Dropbox",tr("There was a problem opening the browser.\nEnsure there is a default browser in your system."));
}

void CCloudStorage::dbAccessToken(const QString &sAuthorizationCode)
{
  //Validate if another network request is already in progress
  if (running()) return;

  //Validate parameter and global methods
  if (sAuthorizationCode.isEmpty() || m_BACodeVerifier.isEmpty()) return;

  //Set state
  m_iAction=ActionDbAccessToken;

  //Extract code & create query
  QString s=QString("code=%1&"\
                    "grant_type=authorization_code&"\
                    "code_verifier=%2&"\
                    "client_id=%3").
            arg(sAuthorizationCode,m_BACodeVerifier,m_pairDbCredentials.first);

  QNetworkRequest Request;
  Request.setUrl(QUrl("https://api.dropboxapi.com/oauth2/token"));
  Request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  Request.setHeader(QNetworkRequest::ContentLengthHeader,s.toUtf8().size());

  //Send request
  m_Manager.post(Request,s.toUtf8());
  setRunning(true);
}

void CCloudStorage::dbDeleteAccessToken()
{
  m_sDbAccessToken.clear();
  setDbauth(false);
}

void CCloudStorage::dbUpload(const QString &sLocal, const QString &sRemote)
{
  //Validate if another network request is already in progress
  if (running()) return;

  //Validate if access token is valid
  if (!dbauth()) return;

  //Validate if file exists
  if (!QFile(sLocal).exists())
  {
    emit uploadResult(false,"Dropbox",tr("Your database has not been created as yet"));
    return;
  }

  //Set state
  m_iAction=ActionDbUpload;

  //Open file and read all content locally
  QFile File(sLocal);
  File.open(QIODevice::ReadOnly);
  m_BABuffer=File.readAll();
  File.close();

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl("https://content.dropboxapi.com/2/files/upload"));
  Request.setHeader(QNetworkRequest::ContentLengthHeader,m_BABuffer.size());
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sDbAccessToken).toUtf8());
  Request.setRawHeader(QString("Dropbox-API-Arg").toUtf8(),QString("{ \"path\": \"%1\",\"mode\":\"overwrite\" }").arg(sRemote).toUtf8());
  Request.setHeader(QNetworkRequest::ContentTypeHeader,"application/octet-stream");

  //Send request
  m_Manager.post(Request,m_BABuffer);
  setRunning(true);
}

void CCloudStorage::dbDownload(const QString &sRemote, const QString &sLocal)
{
  //Validate if another network request is already in progress
  if (running()) return;

  //Validate if access token is valid
  if (!dbauth()) return;

  //Set state
  m_iAction=ActionDbDownload;

  //Save local file's name for later
  m_sLocal=sLocal;

  //Create local directory if doesn't exist
  QFileInfo FileInfo(m_sLocal);
  if (!FileInfo.absoluteDir().exists())
  {
    QDir().mkpath(FileInfo.absolutePath());
  }

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl("https://content.dropboxapi.com/2/files/download"));
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sDbAccessToken).toUtf8());
  Request.setRawHeader(QString("Dropbox-API-Arg").toUtf8(),QString("{ \"path\": \"%1\" }").arg(sRemote).toUtf8());

  //Send request
  m_Manager.get(Request);
  setRunning(true);
}

void CCloudStorage::gdAuthorizationRequest()
{
  //Create code verifier - 47 bytes
  m_BACodeVerifier=QByteArray::number(QRandomGenerator::global()->bounded(10000,99999));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier.append(QByteArray::number(QRandomGenerator::global()->bounded(10000,99999)));
  m_BACodeVerifier=m_BACodeVerifier.toBase64(QByteArray::Base64UrlEncoding);
  m_BACodeVerifier.truncate(m_BACodeVerifier.length()-1);

  //Create code challenge - 43 bytes
  QByteArray BACodeChallenge=QCryptographicHash::hash(m_BACodeVerifier,QCryptographicHash::Sha256).toBase64(QByteArray::Base64UrlEncoding);
  BACodeChallenge.truncate(BACodeChallenge.length()-1);

  //Open browser to request user approval and code
  if (!QDesktopServices::openUrl(QUrl(QString("https://accounts.google.com/o/oauth2/v2/auth?"\
                                              "client_id=%1&"\
                                              "redirect_uri=%2&"\
                                              "response_type=code&"\
                                              "scope=https://www.googleapis.com/auth/drive.file&"\
                                              "code_challenge=%3&"\
                                              "code_challenge_method=S256").
                                      arg(m_pairGdCredentials.first,m_sGdRedirectURI,BACodeChallenge))))
    emit accessError("Google Drive",tr("There was a problem opening the browser.\nEnsure there is a default browser in your system."));
}

void CCloudStorage::gdRefreshToken(const QString &sAuthorizationCode)
{
  //Validate if another network request is already in progress
  if (running()) return;

  //Validate parameter and global methods
  if (sAuthorizationCode.isEmpty() || m_BACodeVerifier.isEmpty()) return;

  //Set state
  m_iAction=ActionGdRefreshToken;

  //Create query
  QString s=QString("client_id=%1&"\
                    "client_secret=%2&"\
                    "code=%3&"\
                    "code_verifier=%4&"\
                    "grant_type=authorization_code&"\
                    "redirect_uri=%5").
            arg(m_pairGdCredentials.first,m_pairGdCredentials.second,sAuthorizationCode,m_BACodeVerifier,m_sGdRedirectURI);

  QNetworkRequest Request;
  Request.setUrl(QUrl("https://oauth2.googleapis.com/token"));
  Request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  Request.setHeader(QNetworkRequest::ContentLengthHeader,s.toUtf8().size());

  //Send request
  m_Manager.post(Request,s.toUtf8());
  setRunning(true);
}

void CCloudStorage::gdCallAPI(bool bUpload, const QString& sLocal, const QString& sRemote)
{
  //Validate if another network request is already in progress
  if (running()) return;

  //Validate if refresh token is valid
  if (!gdauth()) return;

  //Set state
  m_iAction=ActionGdAccessToken;

  //Set parameters for Upload/Download routines
  m_sLocal=sLocal;
  m_sRemote=sRemote;
  m_bUpload=bUpload;

  //Extract code & create query
  QString s=QString("client_id=%1&"\
                    "client_secret=%2&"\
                    "grant_type=refresh_token&"\
                    "refresh_token=%3").
            arg(m_pairGdCredentials.first,m_pairGdCredentials.second,m_sGdRefreshToken);

  QNetworkRequest Request;
  Request.setUrl(QUrl("https://oauth2.googleapis.com/token"));
  Request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  Request.setHeader(QNetworkRequest::ContentLengthHeader,s.toUtf8().size());

  //Send request
  m_Manager.post(Request,s.toUtf8());
  setRunning(true);

  return;
}

void CCloudStorage::gdDeleteAccessToken()
{
  m_sGdRefreshToken.clear();
  setGdauth(false);
}

bool CCloudStorage::gdUpload()
{
  //Validate if file exists
  if (!QFile(m_sLocal).exists())
  {
    emit uploadResult(false,"Google Drive",tr("Your database has not been created as yet"));
    return false;
  }

  //Set state
  m_iAction=ActionGdUpload;

  //Declare variables for Upload or Update
  QFile File(m_sLocal);
  QString sBoundary="!1@2#3$4%5&6*7(8)9";

  m_BABuffer=QString("--%1\r\nContent-Type: application/json\r\n\r\n{\r\n'name': '%2'\r\n}\r\n--%1\r\nContent-Type: application/octet-stream\r\n\r\n").arg(sBoundary,m_sRemote).toUtf8();

  //Open file and read all content locally
  File.open(QIODevice::ReadOnly);
  m_BABuffer.append(File.readAll());
  File.close();

  m_BABuffer.append(QString("\r\n--%1--").arg(sBoundary).toUtf8());

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl(QString("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart")));
  Request.setHeader(QNetworkRequest::ContentTypeHeader,QString("multipart/related; boundary=%1").arg(sBoundary));
  Request.setHeader(QNetworkRequest::ContentLengthHeader,m_BABuffer.size());
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sGdAccessToken).toUtf8());

  //Send request
  m_Manager.post(Request,m_BABuffer);
  return true;
}

bool CCloudStorage::gdDownload()
{
  //Validate if fileId is available
  if (m_sGdFileId.isEmpty())
  {
    emit downloadResult(false,"Google Drive",tr("Your database has not been uploaded as yet"));
    return false;
  }

  //Set state
  m_iAction=ActionGdDownload;

  //Create local directory if doesn't exist
  QFileInfo FileInfo(m_sLocal);
  if (!FileInfo.absoluteDir().exists())
  {
    QDir().mkpath(FileInfo.absolutePath());
  }

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl(QString("https://www.googleapis.com/drive/v3/files/%1?alt=media").arg(m_sGdFileId)));
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sGdAccessToken).toUtf8());

  //Send request
  m_Manager.get(Request);

  return true;
}

bool CCloudStorage::gdDelete(const QString &sId)
{
  if (sId.isEmpty()) return false;

  //Set state
  m_iAction=ActionGdDelete;

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl("https://www.googleapis.com/drive/v3/files/"+sId));
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sGdAccessToken).toUtf8());

  //Send request
  m_Manager.deleteResource(Request);
  return true;
}

void CCloudStorage::replyFinished(QNetworkReply* pReply)
{
  QByteArray BABody=pReply->readAll();

  //** ONEDRIVE - Refresh token
  if (m_iAction==ActionOdRefreshToken)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    if (JsonObject.contains("refresh_token"))
    {
      m_sOdRefreshToken=JsonObject.value("refresh_token").toString();
      setOdauth(true);
    }
    else
    {
      m_sOdRefreshToken.clear();
      setOdauth(false);
      emit accessError("OneDrive",JsonObject.value("error_description").toString());
    }
  }

  //** ONEDRIVE - Access token
  if (m_iAction==ActionOdAccessToken)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    if (JsonObject.contains("access_token"))
    {
      m_sOdAccessToken=JsonObject.value("access_token").toString();
      bool bKeepRunning;
      if (m_bUpload)
        bKeepRunning=odUpload();
      else
        bKeepRunning=odDownloadLink();

      if (!bKeepRunning) setRunning(false);
      pReply->deleteLater();
      return;
    }
    else
    {
      emit accessError("OneDrive",JsonObject.value("error_description").toString());
    }
  }

  //** ONEDRIVE - Upload
  if (m_iAction==ActionOdUpload)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    if ((JsonObject.isEmpty()) || (JsonObject.contains("error")))
    {
      emit uploadResult(false,"OneDrive",JsonObject.value("error").toObject().value("message").toString());
    }
    else
    {
      emit uploadResult(true,"OneDrive");
    }
  }

  //** ONEDRIVE - DownloadLink
  if (m_iAction==ActionOdDownloadLink)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    QString sDownloadUrl=pReply->header(QNetworkRequest::LocationHeader).toString();
    if (JsonObject.contains("error"))
    {
      emit downloadResult(false,"OneDrive",JsonObject.value("error").toObject().value("message").toString());
    }
    else
    {
      bool bKeepRunning=odDownloadContent(sDownloadUrl);
      if (!bKeepRunning) setRunning(false);
      pReply->deleteLater();
      return;
    }
  }

  //** ONEDRIVE - DownloadContent
  if (m_iAction==ActionOdDownloadContent)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    if ((BABody.isEmpty()) || (JsonObject.contains("error")))
    {
      emit downloadResult(false,"OneDrive",JsonObject.value("error").toObject().value("message").toString());
    }
    else
    {
      QFile File(m_sLocal);
      File.open(QIODevice::WriteOnly);
      File.write(BABody);
      File.close();
      emit downloadResult(true,"OneDrive");
    }
  }

  //** DROPBOX - Access token
  if (m_iAction==ActionDbAccessToken)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    if (JsonObject.contains("access_token"))
    {
      m_sDbAccessToken=JsonObject.value("access_token").toString();
      setDbauth(true);
      m_BACodeVerifier.clear();
    }
    else
    {
      m_sDbAccessToken.clear();
      setDbauth(false);
      emit accessError("Dropbox",JsonObject.value("error_description").toString());
    }
  }

  //** DROPBOX - Upload
  if (m_iAction==ActionDbUpload)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    if ((JsonObject.isEmpty()) || (JsonObject.contains("error")))
    {
      emit uploadResult(false,"Dropbox",JsonObject.value("error_summary").toString());
    }
    else
    {
      emit uploadResult(true,"Dropbox");
    }
  }

  //** DROPBOX - Download
  if (m_iAction==ActionDbDownload)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    if ((BABody.isEmpty()) || (JsonObject.contains("error")))
    {
      emit downloadResult(false,"Dropbox",JsonObject.value("error_summary").toString());
    }
    else
    {
      QFile File(m_sLocal);
      File.open(QIODevice::WriteOnly);
      File.write(BABody);
      File.close();
      emit downloadResult(true,"Dropbox");
    }
  }

  //** GOOGLE DRIVE - Refresh token
  if (m_iAction==ActionGdRefreshToken)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    if (JsonObject.contains("refresh_token"))
    {
      m_sGdRefreshToken=JsonObject.value("refresh_token").toString();
      setGdauth(true);
    }
    else
    {
      m_sGdRefreshToken.clear();
      setGdauth(false);
      emit accessError("Google Drive",JsonObject.value("error_description").toString());
    }
  }

  //** GOOGLE DRIVE - Access token
  if (m_iAction==ActionGdAccessToken)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    if (JsonObject.contains("access_token"))
    {
      m_sGdAccessToken=JsonObject.value("access_token").toString();
      bool bKeepRunning;
      if (m_bUpload)
        bKeepRunning=gdUpload();
      else
        bKeepRunning=gdDownload();

      if (!bKeepRunning) setRunning(false);
      pReply->deleteLater();
      return;
    }
    else
    {
      emit accessError("Google Drive",JsonObject.value("error_description").toString());
    }
  }

  //** GOOGLE DRIVE - Upload
  if (m_iAction==ActionGdUpload)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    if ((JsonObject.isEmpty()) || (JsonObject.contains("error")))
    {
      emit uploadResult(false,"Google Drive",JsonObject.value("error").toObject().value("message").toString());
    }
    else
    {
      //Delete previous file
      bool bKeepRunning;
      bKeepRunning=gdDelete(m_sGdFileId);

      //Update parameters for new file
      m_sGdFileId=JsonObject.value("id").toString();
      emit uploadResult(true,"Google Drive");

      if (!bKeepRunning) setRunning(false);
      pReply->deleteLater();
      return;
    }
  }

  //** GOOGLE DRIVE - Download
  if (m_iAction==ActionGdDownload)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    QJsonObject JsonObject=JsonDocument.object();
    if ((BABody.isEmpty()) || (JsonObject.contains("error")))
    {
      emit downloadResult(false,"Google Drive",JsonObject.value("error").toObject().value("message").toString());
    }
    else
    {
      QFile File(m_sLocal);
      File.open(QIODevice::WriteOnly);
      File.write(BABody);
      File.close();
      emit downloadResult(true,"Google Drive");
    }
  }

  //** GOOGLE DRIVE - Delete
  if (m_iAction==ActionGdDelete)
  {
    //No action required
  }

  setRunning(false);
  pReply->deleteLater();
}
