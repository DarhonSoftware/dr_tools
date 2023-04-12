#include "dr_sqlrelationaltablemodel.h"
#include <QtGui>
#include <QtSql>

CSqlRelationalTableModel::CSqlRelationalTableModel(QObject *pObject)
    : QSqlRelationalTableModel(pObject)
{
    m_hashRoles.clear();
}

QVariant CSqlRelationalTableModel::data(const QModelIndex &Index, int iRole) const
{
    if (iRole >= Qt::UserRole)
        return QSqlRelationalTableModel::data(index(Index.row(), iRole - Qt::UserRole));

    return QSqlRelationalTableModel::data(Index, iRole);
}

QHash<int, QByteArray> CSqlRelationalTableModel::roleNames() const
{
    return m_hashRoles;
}

void CSqlRelationalTableModel::setProxyModel(const QString &sTable)
{
    QSqlTableModel Table;
    Table.setTable(sTable);
    QSqlRecord Record = Table.record();

    m_hashRoles.clear();
    for (int i = 0; i < Record.count(); i++)
        m_hashRoles.insert(Qt::UserRole + i, QByteArray("role_") + Record.fieldName(i).toUtf8());

    setTable(sTable);
}
