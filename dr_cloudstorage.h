//Release 1
#ifndef DR_CLOUDSTORAGE_H
#define DR_CLOUDSTORAGE_H

#include <QFile>
#include <QNetworkReply>
#include <QObject>
#include "dr_crypt.h"

class CCloudStorage : public QObject
{
    Q_OBJECT

    enum Action {
        ActionOdRefreshToken,
        ActionOdAccessToken,
        ActionOdUpload,
        ActionOdDownload,
        ActionDbRefreshToken,
        ActionDbAccessToken,
        ActionDbUpload,
        ActionDbDownload,
        ActionGdRefreshToken,
        ActionGdAccessToken,
        ActionGdUpload,
        ActionGdDownload,
        ActionGdDelete
    };

    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool odauth READ odauth WRITE setOdauth NOTIFY odauthChanged)
    Q_PROPERTY(bool dbauth READ dbauth WRITE setDbauth NOTIFY dbauthChanged)
    Q_PROPERTY(bool gdauth READ gdauth WRITE setGdauth NOTIFY gdauthChanged)

    bool m_bRunning;
    bool m_bOdauth;
    bool m_bDbauth;
    bool m_bGdauth;

    //Stored tokens
    QString m_sOdRefreshToken;
    QString m_sDbRefreshToken;
    QString m_sGdRefreshToken;

    //Stored info
    QString m_sGdFileId;

    //Paremeters to set up the service
    QPair<QString, QString> m_pairOdCredentials;
    QPair<QString, QString> m_pairDbCredentials;
    QPair<QString, QString> m_pairGdCredentials;
    QString m_sOdRedirectURI;
    QString m_sDbRedirectURI;
    QString m_sGdRedirectURI;

    //Local internal parameters
    Action m_iAction;
    QString m_sLocal;
    QString m_sRemote;
    QString m_sOdAccessToken;
    QString m_sDbAccessToken;
    QString m_sGdAccessToken;
    bool m_bUpload;
    QByteArray m_BACodeVerifier;

    //Local objects
    QNetworkAccessManager m_Manager;
    CCrypt m_Crypt;
    QByteArray m_BABuffer;

public:
    explicit CCloudStorage(QObject *pObject = nullptr);
    ~CCloudStorage();

    void setRunning(bool bRunning)
    {
        m_bRunning = bRunning;
        emit runningChanged();
    }
    bool running() const { return m_bRunning; }
    void setOdauth(bool bOdauth)
    {
        m_bOdauth = bOdauth;
        emit odauthChanged();
    }
    bool odauth() const { return m_bOdauth; }
    void setDbauth(bool bDbauth)
    {
        m_bDbauth = bDbauth;
        emit dbauthChanged();
    }
    bool dbauth() const { return m_bDbauth; }
    void setGdauth(bool bGdauth)
    {
        m_bGdauth = bGdauth;
        emit gdauthChanged();
    }
    bool gdauth() const { return m_bGdauth; }

    void setOdCredencials(const QString &sKey, const QString &sSignature)
    {
        m_pairOdCredentials.first = sKey;
        m_pairOdCredentials.second = sSignature;
    }
    void setDbCredencials(const QString &sKey, const QString &sSignature)
    {
        m_pairDbCredentials.first = sKey;
        m_pairDbCredentials.second = sSignature;
    }
    void setGdCredencials(const QString &sKey, const QString &sSignature)
    {
        m_pairGdCredentials.first = sKey;
        m_pairGdCredentials.second = sSignature;
    }
    void setOdRedirectURI(const QString &sOdRedirectURI) { m_sOdRedirectURI = sOdRedirectURI; }
    void setDbRedirectURI(const QString &sDbRedirectURI) { m_sDbRedirectURI = sDbRedirectURI; }
    void setGdRedirectURI(const QString &sGdRedirectURI) { m_sGdRedirectURI = sGdRedirectURI; }

signals:
    void runningChanged();
    void odauthChanged();
    void dbauthChanged();
    void gdauthChanged();

    void accessError(const QString &sServer, const QString &sError = QString());
    void uploadResult(bool bOk, const QString &sServer, const QString &sError = QString());
    void downloadResult(bool bOk, const QString &sServer, const QString &sError = QString());

private slots:
    void replyFinished(QNetworkReply *pReply);

private:
    bool odUpload();
    bool odDownload();
    bool dbUpload();
    bool dbDownload();
    bool gdUpload();
    bool gdDownload();
    bool gdDelete(const QString &sId);

public slots:
    QUrl odAuthorizationRequest();
    bool odRefreshToken(const QString &sUrl);
    void odCallAPI(bool bUpload, const QString &sLocal, const QString &sRemote);
    void odDeleteAccessToken();

    QUrl dbAuthorizationRequest();
    bool dbRefreshToken(const QString &sUrl);
    void dbCallAPI(bool bUpload, const QString &sLocal, const QString &sRemote);
    void dbDeleteAccessToken();

    QUrl gdAuthorizationRequest();
    bool gdRefreshToken(const QString &sUrl);
    void gdCallAPI(bool bUpload, const QString &sLocal, const QString &sRemote);
    void gdDeleteAccessToken();
};

#endif // DR_CLOUDSTORAGE_H
