#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>

class QWidget;

namespace Ui
{
  class CAbout;
}

class CAbout : public QDialog
{
  Q_OBJECT
public:
  CAbout(const QString& sIconAbout, const QString& sIconDR, const QString& sAppName,
         const QString& sCaption, const QString& sDescription, const QString& sCopyright,
         const QString &sPrivacy, QWidget *pWidget=0);
  ~CAbout();

private:
  Ui::CAbout *ui;

private slots:
  void on_pPBClose_clicked();
  void on_pPBEdit_clicked();
};

#endif // ABOUT_H
