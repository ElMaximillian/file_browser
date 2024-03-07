#include <QAbstractItemModel>
#include <QDir>
#include <QFileInfo>
#include <QVector>
#include <QTreeView>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>

class CustomDirItem 
{
public:
    CustomDirItem* m_parentItem;

    explicit CustomDirItem(const QString& path, CustomDirItem* parent = nullptr)
        : itemPath(path), m_parentItem(parent) 
    {
        populateChildren();
    }


    ~CustomDirItem() 
    {
        qDeleteAll(childItems);
    }

    CustomDirItem* child(int row) 
    {
        return childItems.value(row);
    }

    int childCount() const 
    {
        return childItems.count();
    }

    int row() const 
    {
        if (m_parentItem)
            return m_parentItem->childItems.indexOf(const_cast<CustomDirItem*>(this));
        return 0;
    }

    QVariant data(int column) const 
    {
        if (column == 0)
            return fileInfo.fileName();
        return QVariant();
    }

    CustomDirItem* parentItem() 
    {
        return m_parentItem;
    }

    QString getPath() const
    {
        return itemPath;
    }

private:
    void populateChildren() 
    {
        QFileInfoList fileInfoList = QDir(itemPath).entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
        for (const QFileInfo& info : fileInfoList) {
            childItems.append(new CustomDirItem(info.filePath(), this));
        }
    }

    QString itemPath;
    
    QVector<CustomDirItem*> childItems;
    QFileInfo fileInfo;
};

class CustomDirModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit CustomDirModel(QObject* parent = nullptr)
        : QAbstractItemModel(parent) 
    {
        rootItem = new CustomDirItem(QDir::rootPath());
    }

    ~CustomDirModel() 
    {
        delete rootItem;
    }

    QModelIndex index(int row, int column, const QModelIndex& parent) const override 
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();

        CustomDirItem* parentItem;

        if (!parent.isValid())
            parentItem = rootItem;
        else
            parentItem = static_cast<CustomDirItem*>(parent.internalPointer());

        CustomDirItem* childItem = parentItem->child(row);
        if (childItem)
            return createIndex(row, column, childItem);
        else
            return QModelIndex();
    }

    QModelIndex parent(const QModelIndex& index) const override 
    {
        if (!index.isValid())
            return QModelIndex();

        CustomDirItem* childItem = static_cast<CustomDirItem*>(index.internalPointer());
        CustomDirItem* parentItem = childItem->parentItem();

        if (parentItem == rootItem)
            return QModelIndex();

        return createIndex(parentItem->row(), 0, parentItem);
    }

    int rowCount(const QModelIndex& parent) const override 
    {
        CustomDirItem* parentItem;
        if (parent.column() > 0)
            return 0;

        if (!parent.isValid())
            parentItem = rootItem;
        else
            parentItem = static_cast<CustomDirItem*>(parent.internalPointer());

        return parentItem->childCount();
    }

    int columnCount(const QModelIndex& parent) const override 
    {
        Q_UNUSED(parent);
        return 1;
    }

    QVariant data(const QModelIndex& index, int role) const override 
    {
        if (!index.isValid())
            return QVariant();

        if (role != Qt::DisplayRole)
            return QVariant();

        CustomDirItem* item = static_cast<CustomDirItem*>(index.internalPointer());

        return item->data(index.column());
    }

    void setRootIndex(const QModelIndex& index) 
    {
        rootIndex = index;
    }

private:
    CustomDirItem* rootItem;
    QModelIndex rootIndex;
};


int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QTreeView fileBrowser;
    CustomDirModel model;
    model.setRootIndex(model.index(0, 0, QModelIndex()));
    fileBrowser.setModel(&model);
    fileBrowser.show();

    return app.exec();
}

