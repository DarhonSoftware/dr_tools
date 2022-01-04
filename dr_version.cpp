#include <QtCore>
#include <QtNetwork>
#include "dr_version.h"

bool g_silverVersion(const QString& sNewActCode, const QString &sType)
{
  QString sActCode=sNewActCode;
  QDate DateSubscription;

  //Load activation code & date from file
  QSettings Settings;
  Settings.beginGroup("Version");
  DateSubscription=QDate::fromJulianDay(Settings.value("key2",2415021).toInt());
  if (sNewActCode.isEmpty()) sActCode=Settings.value("key1","").toString();
  Settings.endGroup();

  //Validate code word
  if (!sActCode.contains(QRegExp("^(\\d|[A-F]|[a-f]){4,4}-(\\d|[A-F]|[a-f]){4,4}-(\\d|[A-F]|[a-f]){4,4}-(\\d|[A-F]|[a-f]){4,4}$"))) return true;

  //Reverse evaluation
  QString sAppCode=g_actCodeToAppCode(sActCode);

  //Validate whether Gold version is activated
  if ((sAppCode.toUpper()==g_appCode(sType,false)) &&
      ((sType.at(2)=='0') ||
       ((sType.at(2)=='1')&&(QDate::currentDate()<=DateSubscription.addMonths(1))) ||
       ((sType.at(2)=='2')&&(QDate::currentDate()<=DateSubscription.addYears(1)))))
    return false;

  return true;
}

void g_registerGoldVersion(const QString &sActCode)
{
  QSettings Settings;
  Settings.beginGroup("Version");
  Settings.setValue("key2",QDate::currentDate().toJulianDay());
  Settings.setValue("key1",sActCode.toUpper());
  Settings.endGroup();
}

QString g_appCode(const QString &sType, bool bCurrentDate)
{
  //Device specific string
  QString sDeviceID="ID";
  QList<QNetworkInterface>lstInterfaces=QNetworkInterface::allInterfaces();
  for (int i=0;i<lstInterfaces.count();i++)
  {
    QString sMac=lstInterfaces.at(i).hardwareAddress();
    if ((!sMac.isEmpty()) && (sMac!="00:00:00:00:00:00"))
    {
      sDeviceID=sMac;
      break;
    }
  }

  //Version specific string
  QString sVer=QCoreApplication::applicationVersion();
  sVer=sVer.left(sVer.indexOf('.'));

  //Monthly/Yearly subscription string
  QString sSubscription;
  if (bCurrentDate)
    sSubscription=QString::number(QDate::currentDate().toJulianDay());
  else
  {
    QSettings Settings;
    Settings.beginGroup("Version");
    sSubscription=Settings.value("key2","").toString();
    Settings.endGroup();
  }

  //Compile Code
  QString sCode=QString("%1*%2*%3*%4*%5").
                arg(QCoreApplication::applicationName(),
                ((sType.at(0)=='1')?sDeviceID:"X"),
                ((sType.at(1)=='1')?sVer:"X"),
                ((sType.at(2)=='1')?sSubscription:"X"),
                ((sType.at(2)=='2')?sSubscription:"X"));

  //Create crypto code
  QByteArray BACode=sCode.toUtf8();
  BACode=QCryptographicHash::hash(BACode,QCryptographicHash::Md5);
  BACode=BACode.toHex().toUpper();

  //Write AppCode in human version and cut half the original code
  sCode.clear();
  for (int i=0,j=0; i<BACode.count();i+=2,j++)
  {
    if (((j%4) == 0)&&(j>0)) sCode+="-";
    sCode+=BACode.at(i);
  }

  return sCode;
}

QString g_actCodeToAppCode(const QString& sActCode)
{
  bool bOk;

  //Validate
  if (!sActCode.contains(QRegExp("^(\\d|[A-F]|[a-f]){4,4}-(\\d|[A-F]|[a-f]){4,4}-(\\d|[A-F]|[a-f]){4,4}-(\\d|[A-F]|[a-f]){4,4}$"))) return "";

  //Process the first 16 bits - shift right circular 1
  quint16 u16Code1=static_cast<quint16>(sActCode.left(4).toLong(&bOk,16));
  u16Code1= static_cast<quint16>(u16Code1>>1) | static_cast<quint16>((u16Code1 & 0x0001)<<15);

  //Process the second 16 bits - shift right circular 3
  int i=sActCode.indexOf(QChar('-'));
  quint16 u16Code2=static_cast<quint16>(sActCode.mid(i+1,4).toLong(&bOk,16));
  u16Code2= static_cast<quint16>(u16Code2>>3) | static_cast<quint16>((u16Code2 & 0x0007)<<13);

  //Process the third 16 bits - shift right circular 5
  i=sActCode.indexOf(QChar('-'),i+1);
  quint16 u16Code3=static_cast<quint16>(sActCode.mid(i+1,4).toLong(&bOk,16));
  u16Code3= static_cast<quint16>(u16Code3>>5) | static_cast<quint16>((u16Code3 & 0x001F)<<11);

  //Process the fourth 16 bits - shift right circular 7
  i=sActCode.indexOf(QChar('-'),i+1);
  quint16 u16Code4=static_cast<quint16>(sActCode.mid(i+1,4).toLong(&bOk,16));
  u16Code4= static_cast<quint16>(u16Code4>>7) | static_cast<quint16>((u16Code4 & 0x007F)<<9);

  //Evaluate AppCode
  QString sAppCode=QString("%1-%2-%3-%4").arg(u16Code1,4,16,QChar('0')).arg(u16Code2,4,16,QChar('0'))
                   .arg(u16Code3,4,16,QChar('0')).arg(u16Code4,4,16,QChar('0'));

  //Write AppCode as result
  return (sAppCode.toUpper());
}

QString g_appCodeToActCode(const QString& sAppCode)
{
  bool bOk;

  //Validate
  if (!sAppCode.contains(QRegExp("^(\\d|[A-F]|[a-f]){4,4}-(\\d|[A-F]|[a-f]){4,4}-(\\d|[A-F]|[a-f]){4,4}-(\\d|[A-F]|[a-f]){4,4}$"))) return "";

  //Process the first 16 bits - shift left circular 1
  quint16 u16Code1=static_cast<quint16>(sAppCode.left(4).toLong(&bOk,16));
  u16Code1= static_cast<quint16>(u16Code1<<1) | static_cast<quint16>((u16Code1 & 0x8000)>>15);

  //Process the second 16 bits - shift left circular 3
  int i=sAppCode.indexOf(QChar('-'));
  quint16 u16Code2=static_cast<quint16>(sAppCode.mid(i+1,4).toLong(&bOk,16));
  u16Code2= static_cast<quint16>(u16Code2<<3) | static_cast<quint16>((u16Code2 & 0xE000)>>13);

  //Process the third 16 bits - shift left circular 5
  i=sAppCode.indexOf(QChar('-'),i+1);
  quint16 u16Code3=static_cast<quint16>(sAppCode.mid(i+1,4).toLong(&bOk,16));
  u16Code3= static_cast<quint16>(u16Code3<<5) | static_cast<quint16>((u16Code3 & 0xF800)>>11);

  //Process the fourth 16 bits - shift left circular 7
  i=sAppCode.indexOf(QChar('-'),i+1);
  quint16 u16Code4=static_cast<quint16>(sAppCode.mid(i+1,4).toLong(&bOk,16));
  u16Code4= static_cast<quint16>(u16Code4<<7) | static_cast<quint16>((u16Code4 & 0xFE00)>>9);

  //Evaluate ActCode
  QString sActCode=QString("%1-%2-%3-%4").arg(u16Code1,4,16,QChar('0')).arg(u16Code2,4,16,QChar('0'))
                   .arg(u16Code3,4,16,QChar('0')).arg(u16Code4,4,16,QChar('0'));

  //Write ActCode code as result
  return (sActCode.toUpper());
}

