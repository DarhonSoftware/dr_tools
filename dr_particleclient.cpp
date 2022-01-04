#include <QtCore>
#include <QtNetwork>
#include "dr_particleclient.h"

#define TOKEN_LENGTH           40
#define ERROR_RUNNING          "Another request in progress"
#define ERROR_UNREACHABLE      "Server unreachable"
#define ERROR_UNDEFINED        "Undefined"

CParticleClient::CParticleClient(QObject *pObject) :  QObject(pObject)
{
  //Initialize control parameters
  setRunning(false);
  m_sAccessToken.clear();

  //Initialize subscribed events parameters
  m_pFirstReply=0;
  m_BAEvent.clear();
  m_BAData.clear();

  //Initialize response parameters
  m_sLastError.clear();
  m_lstDevices.clear();
  m_VariableValue.clear();
  m_iFunctionValue=0;

  //Initialize objects
  m_lstReplies.clear();

  //Connect Network Manager
  connect(&m_Manager, &QNetworkAccessManager::finished,this, &CParticleClient::replyFinished);
}

CParticleClient::~CParticleClient()
{
  for (int i=0; i<m_lstReplies.size(); i++)
  {
    delete m_lstReplies.at(i);
  }
}

void CParticleClient::setToken(const QString& sToken)
{
  if (sToken.size()==TOKEN_LENGTH) m_sAccessToken=sToken;
}

bool CParticleClient::listDevices()
{
  //Reset last error
  m_sLastError.clear();

  //Validate if another network request is already in progress
  if (running())
  {
    m_sLastError=tr(ERROR_RUNNING);
    return false;
  }

  //Set Action
  m_iAction=ActionListDevices;

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl("https://api.particle.io/v1/devices"));
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sAccessToken).toUtf8());

  //Send request
  m_Manager.get(Request);
  setRunning(true);

  //Wait until process is attended by replyFinished
  m_EventLoop.exec();
  if ((!m_sLastError.isEmpty()) && (m_sLastError!=tr(ERROR_RUNNING))) return false;

  return true;
}

bool CParticleClient::variable(const QString& sDeviceId,const QString& sVariable)
{
  //Reset last error
  m_sLastError.clear();

  //Validate if another network request is already in progress
  if (running())
  {
    m_sLastError=tr(ERROR_RUNNING);
    return false;
  }

  //Set Action
  m_iAction=ActionVariable;

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl(QString("https://api.particle.io/v1/devices/%1/%2").arg(sDeviceId).arg(sVariable)));
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sAccessToken).toUtf8());

  //Send request
  m_Manager.get(Request);
  setRunning(true);

  //Wait until process is attended by replyFinished
  m_EventLoop.exec();
  if ((!m_sLastError.isEmpty()) && (m_sLastError!=tr(ERROR_RUNNING))) return false;

  return true;
}

bool CParticleClient::function(const QString& sDeviceId,const QString& sFunction,const QString& sArgument)
{
  //Reset last error
  m_sLastError.clear();

  //Validate if another network request is already in progress
  if (running())
  {
    m_sLastError=tr(ERROR_RUNNING);
    return false;
  }

  //Set Action
  m_iAction=ActionFunction;

  //Prepare data
  QString s="args="+sArgument;

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl(QString("https://api.particle.io/v1/devices/%1/%2").arg(sDeviceId).arg(sFunction)));
  Request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  Request.setHeader(QNetworkRequest::ContentLengthHeader,s.toUtf8().size());
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sAccessToken).toUtf8());

  //Send request
  m_Manager.post(Request,s.toUtf8());
  setRunning(true);

  //Wait until process is attended by replyFinished
  m_EventLoop.exec();
  if ((!m_sLastError.isEmpty()) && (m_sLastError!=tr(ERROR_RUNNING))) return false;

  return true;
}

bool CParticleClient::publishEvent(const QString& sEventName, const QString& sData, bool bPrivate, int iTtl)
{
  //Reset last error
  m_sLastError.clear();

  //Validate if another network request is already in progress
  if (running())
  {
    m_sLastError=tr(ERROR_RUNNING);
    return false;
  }

  //Set Action
  m_iAction=ActionPublishEvent;

  //Prepare data
  QString s=QString("name=%1&"\
                    "data=%2&"\
                    "private=%3&"\
                    "ttl=%4").
            arg(sEventName).arg(sData).arg((bPrivate) ? "true" : "false").arg(iTtl);

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl("https://api.particle.io/v1/devices/events"));
  Request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  Request.setHeader(QNetworkRequest::ContentLengthHeader,s.toUtf8().size());
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sAccessToken).toUtf8());

  //Send request
  m_Manager.post(Request,s.toUtf8());
  setRunning(true);

  //Wait until process is attended by replyFinished
  m_EventLoop.exec();
  if ((!m_sLastError.isEmpty()) && (m_sLastError!=tr(ERROR_RUNNING))) return false;

  return true;
}

bool CParticleClient::subscribeEvent(Event iType, const QString& sEventName, const QString& sDeviceId)
{
  //Reset last error
  m_sLastError.clear();

  //Validate if another network request is already in progress
  if (running())
  {
    m_sLastError=tr(ERROR_RUNNING);
    return false;
  }

  //Set Action & FirstEvent
  m_iAction=ActionSubscribeEvent;

  //Prepare data
  QString sUrl="";
  switch (iType) {
    case EventGlobal:
      sUrl=QString("https://api.particle.io/v1/events/%1").arg(sEventName);
      break;
    case EventOwn:
      sUrl=QString("https://api.particle.io/v1/devices/events/%1").arg(sEventName);
      break;
    case EventDevice:
      sUrl=QString("https://api.particle.io/v1/devices/%1/events/%2").arg(sDeviceId).arg(sEventName);
      break;
    default:
      break;
  }

  //Prepare Request
  QNetworkRequest Request;
  Request.setUrl(QUrl(sUrl));
  Request.setRawHeader(QString("Authorization").toUtf8(),QString("Bearer %1").arg(m_sAccessToken).toUtf8());

  //Send request
  m_pFirstReply=m_Manager.get(Request);
  connect(m_pFirstReply,&QNetworkReply::readyRead,this,&CParticleClient::eventReceived);
  m_lstReplies.append(m_pFirstReply);
  setRunning(true);

  //Wait until process is attended by eventReceived
  m_EventLoop.exec();
  if ((!m_sLastError.isEmpty()) && (m_sLastError!=tr(ERROR_RUNNING))) return false;

  return true;
}

void CParticleClient::replyFinished(QNetworkReply* pReply)
{
  QByteArray BABody=pReply->readAll();

  //************* LIST DEVICES ***************************//
  if (m_iAction==ActionListDevices)
  {
    //Reset response
    m_lstDevices.clear();

    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    if (JsonDocument.isNull() || JsonDocument.isEmpty())
    {
      m_sLastError=tr(ERROR_UNREACHABLE);
    }
    else if (JsonDocument.isObject())
    {
      QJsonObject JsonObject=JsonDocument.object();
      if (JsonObject.contains("error"))
        m_sLastError=JsonObject.value("error").toString();
      else
        m_sLastError=tr(ERROR_UNDEFINED);
    }
    else if (JsonDocument.isArray())
    {
      QJsonArray JsonArray=JsonDocument.array();
      for (int i=0;i<JsonArray.size();i++)
      {
        SDevice Device;
        if (JsonArray.at(i).isObject())
        {
          QJsonObject JsonObjDevice=JsonArray.at(i).toObject();
          Device.bConnected=JsonObjDevice.value("connected").toBool();
          Device.DateLastHeard=QDateTime::fromString(JsonObjDevice.value("last_heard").toString(),Qt::ISODate);
          Device.sDeviceId=JsonObjDevice.value("id").toString();
          Device.sName=JsonObjDevice.value("name").toString();
          m_lstDevices.append(Device);
        }
      }
    }
  }

  //************* VARIABLE ***************************//
  if (m_iAction==ActionVariable)
  {
    //Reset response
    m_VariableValue.clear();

    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    if (JsonDocument.isNull() || JsonDocument.isEmpty())
    {
      m_sLastError=tr(ERROR_UNREACHABLE);
    }
    else if (JsonDocument.isObject())
    {
      QJsonObject JsonObject=JsonDocument.object();
      if (JsonObject.contains("error"))
      {
        m_sLastError=JsonObject.value("error").toString();
      }
      else
      {
        m_VariableValue=JsonObject.value("result").toVariant();
      }
    }
  }

  //************* FUNCTION ***************************//
  if (m_iAction==ActionFunction)
  {
    //Reset response
    m_iFunctionValue=0;

    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    if (JsonDocument.isNull() || JsonDocument.isEmpty())
    {
      m_sLastError=tr(ERROR_UNREACHABLE);
    }
    else if (JsonDocument.isObject())
    {
      QJsonObject JsonObject=JsonDocument.object();
      if (JsonObject.contains("error"))
      {
        m_sLastError=JsonObject.value("error").toString();
      }
      else
      {
        m_iFunctionValue=JsonObject.value("return_value").toInt();
      }
    }
  }

  //************* PUBLISH EVENT **********************//
  if (m_iAction==ActionPublishEvent)
  {
    //Validate operation
    QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
    if (JsonDocument.isNull() || JsonDocument.isEmpty())
    {
      m_sLastError=tr(ERROR_UNREACHABLE);
    }
    else if (JsonDocument.isObject())
    {
      QJsonObject JsonObject=JsonDocument.object();
      if (JsonObject.contains("error"))
      {
        m_sLastError=JsonObject.value("error").toString();
      }
      else
      {
        //No action
      }
    }
  }

  //************* SUBSCRIBE EVENT **********************//
  if ((m_iAction==ActionSubscribeEvent)&&(m_pFirstReply==0))
  {
    m_lstReplies.removeOne(pReply);
    pReply->deleteLater();
    return;
  }
  if ((m_iAction==ActionSubscribeEvent)&&(m_pFirstReply==pReply))
  {
    m_lstReplies.removeOne(pReply);
    m_pFirstReply=0;
    m_sLastError=tr(ERROR_UNREACHABLE);
  }

  setRunning(false);
  m_EventLoop.quit();
  pReply->deleteLater();
}

void CParticleClient::eventReceived()
{
  QNetworkReply *pReply=qobject_cast<QNetworkReply*>(sender());
  QByteArray BABody=pReply->readAll();

  //Validate operation
  QJsonDocument JsonDocument=QJsonDocument::fromJson(BABody);
  if (JsonDocument.isObject())
  {
    QJsonObject JsonObject=JsonDocument.object();
    m_sLastError=JsonObject.value("error").toString();
  }

  //Process events
  else
  {
    int iIndex1;
    int iIndex2;
    const char* pEventText="event: ";
    const char* pDataText="data: ";

    do
    {
      //Extract event name
      if (m_BAEvent.isEmpty())
      {
        m_BAEvent=BABody;
        iIndex1=m_BAEvent.indexOf(pEventText);
        iIndex2=m_BAEvent.indexOf("\n",iIndex1);
        if ((iIndex1!=-1)&&(iIndex2!=-1))
        {
          m_BAEvent=m_BAEvent.mid(iIndex1+QByteArray(pEventText).size(),iIndex2-iIndex1-QByteArray(pEventText).size());
          BABody=BABody.right(BABody.size()-iIndex2-1);
        }
        else
        {
          m_BAEvent.clear();
        }
      }

      //Extract data value
      if (m_BAData.isEmpty())
      {
        m_BAData=BABody;
        iIndex1=m_BAData.indexOf(pDataText);
        iIndex2=m_BAData.indexOf("\n\n",iIndex1);
        if ((iIndex1!=-1)&&(iIndex2!=-1))
        {
          m_BAData=m_BAData.mid(iIndex1+QByteArray(pDataText).size(),iIndex2-iIndex1-QByteArray(pDataText).size());
          QJsonDocument JsonDocument=QJsonDocument::fromJson(m_BAData);
          if (JsonDocument.isObject())
          {
            QJsonObject JsonObject=JsonDocument.object();
            m_BAData=JsonObject.value("data").toString().toUtf8();
          }
          BABody=BABody.right(BABody.size()-iIndex2-2);
        }
        else
        {
          m_BAData.clear();
        }
      }

      //Share collected event
      if ((!m_BAEvent.isEmpty()) && (!m_BAData.isEmpty()))
      {
        emit subscribedEvent(m_BAEvent,m_BAData);
        m_BAEvent.clear();
        m_BAData.clear();
      }
    } while (BABody.contains(pEventText) || BABody.contains(pDataText));
  }

  if (m_pFirstReply==pReply)
  {
    setRunning(false);
    m_pFirstReply=0;
    m_EventLoop.quit();
  }
}
