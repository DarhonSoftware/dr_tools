#include <QtGui>
#include <QtWidgets>
#include "ui_dr_about.h"
#include "dr_about.h"
#include "dr_profile.h"
#ifdef SILVER_VERSION
#include "dr_version.h"
#endif

#define SILVER_TEXT QT_TRANSLATE_NOOP("Global","Silver version")
#define GOLD_TEXT   QT_TRANSLATE_NOOP("Global","Gold version")

CAbout::CAbout(const QString& sIconAbout, const QString& sIconDR, const QString& sAppName,
               const QString& sCaption, const QString& sDescription, const QString& sCopyright,
               const QString& sPrivacy, QWidget *pWidget) :
    QDialog(pWidget),
    ui(new Ui::CAbout)
{
  ui->setupUi(this);

  //Setup children
  ui->pLEActCode->setInputMethodHints(Qt::ImhNoAutoUppercase);
  ui->pLPrivacy->setOpenExternalLinks(true);

  //Set visibility
  ui->pLEActCode->hide();
  ui->pPBClose->hide();
  if (sPrivacy.isEmpty()) ui->pLPrivacy->hide();
#ifdef GOLD_VERSION
  ui->pPBEdit->hide();
#endif

  //Set icons
  ui->pLAboutIcon->setPixmap(QPixmap(sIconAbout));
  ui->pLDRIcon->setPixmap(QPixmap(sIconDR));

  //Writes information
  ui->pLTitle->setText(QString("<b>%1 %2</b>").arg(sAppName).arg(qApp->applicationVersion()));
  ui->pLQtVersion->setText(QString("(Qt - %1)").arg(QT_VERSION_STR));
  ui->pLCaption->setText(sCaption);
  ui->pLDescription->setText(sDescription);
  ui->pLCopyright->setText(sCopyright);
  ui->pLWebsite->setText(tr("Help & Support") + " - " + QCoreApplication::organizationDomain());
  ui->pLPrivacy->setText(sPrivacy);
#ifdef SILVER_VERSION
  ui->pLAppCode->setText(tr("Application Code:") + " " + g_appCode());
#endif

  //Load activation code from file
#ifdef SILVER_VERSION
  QSettings Settings;
  Settings.beginGroup("Version");
  QString sActCode=Settings.value("key1","X").toString();
  Settings.endGroup();
  if (g_silverVersion(sActCode))
    ui->pLVersion->setText(QCoreApplication::translate("Global",SILVER_TEXT));
  else
  {
    ui->pLVersion->setText(QCoreApplication::translate("Global",GOLD_TEXT));
    ui->pPBEdit->hide();
  }
#else
  ui->pLVersion->setText(QCoreApplication::translate("Global",GOLD_TEXT));
#endif

  //Get application font for reference
  QFont Font=qApp->font();
  int iSize=QFontInfo(Font).pixelSize();
  int iSizeSmall, iSizeNormal, iSizeLarge;

#ifdef MOBILE_PLATFORM
  iSizeSmall=iSize-3*iSize/10;
  iSizeNormal=iSize-2*iSize/10;
  iSizeLarge=iSize;
#else
  iSizeSmall=iSize-2*iSize/10;
  iSizeNormal=iSize;
  iSizeLarge=iSize+2*iSize/10;
#endif

  //Set font for first line
  Font.setPixelSize(iSizeLarge);
  ui->pLTitle->setFont(Font);

  //Set font for middle line
  Font.setPixelSize(iSizeNormal);
  ui->pLQtVersion->setFont(Font);
  ui->pLCaption->setFont(Font);
  ui->pLDescription->setFont(Font);
  ui->pLEActCode->setFont(Font);
  ui->pLVersion->setFont(Font);

  //Set font for last line
  Font.setPixelSize(iSizeSmall);
  ui->pLAppCode->setFont(Font);
  ui->pLCopyright->setFont(Font);
  ui->pLWebsite->setFont(Font);
  ui->pLPrivacy->setFont(Font);

  //Resize the window to optimaize the platform style
  adjustSize();
}

CAbout::~CAbout()
{
  delete ui;
}


void CAbout::on_pPBEdit_clicked()
{
#ifdef SILVER_VERSION
  //Change to Edit activation code mode
  if (ui->pLVersion->isVisible())
  {
    ui->pLEActCode->clear();;
    ui->pLVersion->hide();
    ui->pLEActCode->show();
    ui->pPBClose->show();
    ui->pPBEdit->setText(tr("Save"));
    ui->pLEActCode->setFocus();
  }

  //Save new activation code if it is correct and change label
  else
  {
    QString sNewActCode=ui->pLEActCode->text();

    if (!g_silverVersion(sNewActCode))
    {
      QSettings Settings;
      Settings.beginGroup("Version");
      Settings.setValue("key1",sNewActCode.toUpper());
      Settings.endGroup();

      ui->pLVersion->show();
      ui->pLEActCode->hide();
      ui->pPBClose->hide();
      ui->pPBEdit->hide();
      ui->pPBEdit->setText(tr("Edit"));
      ui->pLVersion->setText(QCoreApplication::translate("Global",GOLD_TEXT));
      ui->pPBClose->setFocus();
    }
    else
    {
      ui->pLEActCode->setFocus();
    }
  }
#endif
}

void CAbout::on_pPBClose_clicked()
{
#ifdef SILVER_VERSION
  ui->pLVersion->show();
  ui->pLEActCode->hide();
  ui->pPBClose->hide();
  ui->pPBEdit->setText(tr("Edit"));
  ui->pPBClose->setFocus();
#endif
}

