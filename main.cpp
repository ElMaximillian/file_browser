#include <QApplication>
#include <QAbstractItemModel>
#include <QTreeView>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QInputDialog>
#include <QMessageBox>

class FileSystemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum CustomRoles {
        FilePathRole = Qt::UserRole + 1
    };
    explicit FileSystemModel(QObject* parent = nullptr)
        : QAbstractItemModel(parent)
    {
        rootPath = QDir::rootPath();
        rootItem = new FileItem(rootPath);
        setupModelData(rootItem);
    }

    ~FileSystemModel() override
    {
        delete rootItem;
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        if (role != Qt::DisplayRole)
            return QVariant();

        FileItem* item = static_cast<FileItem*>(index.internalPointer());

        return item->data(index.column());
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
            return rootItem->data(section);

        return QVariant();
    }

    QModelIndex index(int row, int column, const QModelIndex& parent) const override
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();

        FileItem* parentItem = parent.isValid() ? static_cast<FileItem*>(parent.internalPointer()) : rootItem;
        FileItem* childItem = parentItem->child(row);
        if (childItem)
            return createIndex(row, column, childItem);
        else
            return QModelIndex();
    }

    QModelIndex parent(const QModelIndex& index) const override
    {
        if (!index.isValid())
            return QModelIndex();

        FileItem* childItem = static_cast<FileItem*>(index.internalPointer());
        FileItem* parentItem = childItem->parentItem;

        if (parentItem == rootItem)
            return QModelIndex();

        return createIndex(parentItem->row(), 0, parentItem);
    }

    int rowCount(const QModelIndex& parent) const override
    {
        FileItem* parentItem = parent.isValid() ? static_cast<FileItem*>(parent.internalPointer()) : rootItem;
        return parentItem->childCount();
    }

    int columnCount(const QModelIndex& parent) const override
    {
        Q_UNUSED(parent)
            return 1;
    }

    void setRootPath(const QString& path)
    {
        if (rootPath != path) {
            rootPath = path;
            delete rootItem;
            rootItem = new FileItem(rootPath);
            setupModelData(rootItem);
        }
    }

private:
    class FileItem
    {
    public:
        explicit FileItem(const QString& path, FileItem* parent = nullptr)
            : filePath(path), parentItem(parent)
        {
            fileInfo = QFileInfo(path);
        }

        ~FileItem()
        {
            qDeleteAll(childItems);
        }

        void appendChild(FileItem* item)
        {
            childItems.append(item);
        }

        FileItem* child(int row)
        {
            return childItems.value(row);
        }

        int childCount() const
        {
            return childItems.count();
        }

        QVariant data(int column) const
        {
            if (column == 0)
                return fileInfo.fileName();
            else
                return QVariant();
        }

        int row() const
        {
            if (parentItem)
                return parentItem->childItems.indexOf(const_cast<FileItem*>(this));
            return 0;
        }

        FileItem* parentItem;
        QString filePath;
        QFileInfo fileInfo;
        QList<FileItem*> childItems;
    };

    QVariant fileInfoData(const QModelIndex& index) const
    {
        FileItem* item = static_cast<FileItem*>(index.internalPointer());

        switch (index.column()) {
        case 0:
            return item->fileInfo.fileName();
        default:
            return QVariant();
        }
    }

    void setupModelData(FileItem* parent)
    {
        QDir directory(parent->filePath);
        QFileInfoList fileList = directory.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QFileInfo& fileInfo : fileList) {
            FileItem* item = new FileItem(fileInfo.filePath(), parent);
            parent->appendChild(item);
            if (fileInfo.isDir())
                setupModelData(item);
        }
    }


    FileItem* rootItem;
    QString rootPath;
};

class FileBrowser : public QTreeView {
public:
    FileBrowser(QWidget* parent = nullptr) : QTreeView(parent)
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QTreeView::customContextMenuRequested, this, &FileBrowser::showContextMenu);
    }

protected:
    void showContextMenu(const QPoint& pos)
    {
        QModelIndex index = indexAt(pos);
        if (index.isValid())
        {
            QMenu contextMenu(tr("Context menu"), this);
            QAction* editFileAction = new QAction(tr("Change file"), this);
            connect(editFileAction, &QAction::triggered, this, &FileBrowser::editFile);
            contextMenu.addAction(editFileAction);

            QAction* deleteFileAction = new QAction(tr("Delete file"), this);
            connect(deleteFileAction, &QAction::triggered, this, &FileBrowser::deleteFile);
            contextMenu.addAction(deleteFileAction);

            contextMenu.exec(viewport()->mapToGlobal(pos));
        }
    }

    void editFile() 
    {
        QModelIndex index = currentIndex();
        if (index.isValid()) 
        {
            QString filePath = model()->data(index, FileSystemModel::FilePathRole).toString();
            QFileInfo fileInfo(filePath);
            QString parentPath = fileInfo.absoluteDir().path();

            QString newName = QInputDialog::getText(this, tr("Rename"), tr("New name:"), QLineEdit::Normal, fileInfo.fileName());
            if (!newName.isEmpty())
            {
                QString newFilePath = parentPath + QDir::separator() + newName;

                if (QFile::rename(filePath, newFilePath))
                    model()->setData(index, newFilePath, FileSystemModel::FilePathRole);
                else
                    QMessageBox::warning(this, tr("Error"), tr("Failed to rename"));
            }
        }
    }

    void deleteFile() {
        QModelIndex index = currentIndex();
        if (index.isValid())
        {
            QString filePath = model()->data(index, FileSystemModel::FilePathRole).toString();
            QFile file(filePath);
            if (file.remove())
                model()->removeRow(index.row(), index.parent());
            else
            {
                QDir dir(filePath);
                if (dir.exists())
                {
                    if (dir.removeRecursively())
                        model()->removeRow(index.row(), index.parent());
                    else
                        QMessageBox::warning(this, tr("Error"), tr("Failed to delete directory."));
                }
            }
        }
    }
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    FileBrowser fileBrowser;
    FileSystemModel model;
    model.setRootPath(QDir::rootPath());
    fileBrowser.setModel(&model);
    fileBrowser.setRootIndex(model.index(0, 0, QModelIndex()));
    fileBrowser.show();

    return app.exec();
}


