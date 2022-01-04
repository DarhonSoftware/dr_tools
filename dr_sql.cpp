#include <QtCore>
#include <QtSql>
#include "dr_sql.h"

qulonglong g_rowCount(const QSqlTableModel& Model)
{
  //Maximum value returning by rowCount() is 256, which is the amount of rows catched in the buffer
  //This function returns the correct rowCount based on SQL query in the table

  QSqlQuery Query;
  if (Model.filter().isEmpty())
    Query.exec("SELECT count() FROM "+Model.tableName());
  else
    Query.exec("SELECT count() FROM "+Model.tableName()+" WHERE "+Model.filter());

  if (Query.next()) return Query.value(0).toULongLong();
  return 0;
}

bool g_findRecordFromTable(const QSqlTableModel& Model, const QString& sSearchField, const QString& sSearchValue, const QString& sResultField, QVariant *pResultValue)
{
  return g_findRecordFromTable(Model.tableName(),sSearchField,sSearchValue,sResultField,pResultValue,Model.filter());
}

bool g_findRecordFromTable(const QString& sModel, const QString& sSearchField, const QString& sSearchValue, const QString& sResultField, QVariant *pResultValue, const QString& sFilter)
{
  QSqlQuery Query;
  Query.setForwardOnly(true);

  QString sField=sResultField;
  if (sField.isEmpty()) sField=sSearchField;

  if (sFilter.isEmpty())
    Query.exec("SELECT "+ sField +" FROM " + sModel + " WHERE " + sSearchField + " = '" + sSearchValue + "'");
  else
    Query.exec("SELECT "+ sField +" FROM " + sModel + " WHERE " + sSearchField + " = '" + sSearchValue + "' AND " + sFilter);

  if (Query.next())
  {
    if (pResultValue) *pResultValue=Query.value(0);
    return true;
  }

  return false;
}

void g_fetchAllData(QSqlTableModel *pModel)
{
  //Maximum rows catched in buffer is 256. So it fetch all data.
  while (pModel->canFetchMore()) pModel->fetchMore();
}
