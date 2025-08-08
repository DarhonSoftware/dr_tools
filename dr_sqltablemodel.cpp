//Release 3
#include "dr_sqltablemodel.h"
#include <QSqlRecord>

CSqlTableModel::CSqlTableModel(QObject *pObject)
    : QSqlTableModel(pObject)
{
    m_hashRoles.clear();
}

QVariant CSqlTableModel::data(const QModelIndex &Index, int iRole) const
{
    if (iRole >= Qt::UserRole)
        return QSqlTableModel::data(index(Index.row(), iRole - Qt::UserRole));

    return QSqlTableModel::data(Index, iRole);
}

QHash<int, QByteArray> CSqlTableModel::roleNames() const
{
    return m_hashRoles;
}

void CSqlTableModel::setProxyModel(const QString &sTable)
{
    QSqlTableModel Table;
    Table.setTable(sTable);
    QSqlRecord Record = Table.record();

    m_hashRoles.clear();
    for (int i = 0; i < Record.count(); i++)
        m_hashRoles.insert(Qt::UserRole + i, QByteArray("role_") + Record.fieldName(i).toUtf8());

    setTable(sTable);
}
