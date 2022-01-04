#ifndef DR_SQL_H
#define DR_SQL_H

#include <QtCore>

class QSqlTableModel;
class QVariant;
class QString;

/***** Global constats *****/


/***** Global enums *****/


/***** Global functions *****/

qulonglong g_rowCount(const QSqlTableModel& Model);
bool g_findRecordFromTable(const QSqlTableModel& Model, const QString& sSearchField, const QString& sSearchValue, const QString& sResultField=QString(), QVariant *pResultValue=nullptr);
bool g_findRecordFromTable(const QString& sModel, const QString& sSearchField, const QString& sSearchValue, const QString& sResultField=QString(), QVariant *pResultValue=nullptr, const QString &sFilter=QString());
void g_fetchAllData(QSqlTableModel *pModel);

#endif // DR_SQL_H
