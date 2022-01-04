#ifndef SQLRELATIONALTABLEMODEL_H
#define SQLRELATIONALTABLEMODEL_H

#include <QSqlRelationalTableModel>

class CSqlRelationalTableModel : public QSqlRelationalTableModel
{
  Q_OBJECT

  QHash<int, QByteArray> m_hashRoles;

public:
  explicit CSqlRelationalTableModel(QObject *pObject=nullptr);
  QVariant	data (const QModelIndex & Index, int iRole = Qt::DisplayRole) const;
  QHash<int, QByteArray> roleNames() const;
  void setProxyModel(const QString& sTable);

signals:

public slots:

};

#endif // SQLRELATIONALTABLEMODEL_H
