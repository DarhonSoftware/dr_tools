//Release 3
#ifndef SQLTABLEMODEL_H
#define SQLTABLEMODEL_H

#include <QSqlTableModel>

class CSqlTableModel : public QSqlTableModel
{
    Q_OBJECT

    QHash<int, QByteArray> m_hashRoles;

public:
    explicit CSqlTableModel(QObject *pObject = nullptr);
    QVariant data(const QModelIndex &Index, int iRole = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;
    void setProxyModel(const QString &sTable);

signals:

public slots:
};

#endif // SQLTABLEMODEL_H
