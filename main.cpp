#include <QApplication>
#include <QFileSystemModel>
#include <QTreeView>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QFile>
#include <QProcess>
#include <QDesktopServices>
#include <QInputDialog>
#include <QMessageBox>

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
            QString filePath = model()->data(index, QFileSystemModel::FilePathRole).toString();
            QFileInfo fileInfo(filePath);
            QString parentPath = fileInfo.absoluteDir().path();

            QString newName = QInputDialog::getText(this, tr("Rename"), tr("New name:"), QLineEdit::Normal, fileInfo.fileName());
            if (!newName.isEmpty())
            {
                QString newFilePath = parentPath + QDir::separator() + newName;

                if (QFile::rename(filePath, newFilePath))
                    model()->setData(index, newFilePath, QFileSystemModel::FilePathRole);
                else
                    QMessageBox::warning(this, tr("Error"), tr("Failed to rename"));
            }
        }
    }

    void deleteFile() {
        QModelIndex index = currentIndex();
        if (index.isValid())
        {
            QString filePath = model()->data(index, QFileSystemModel::FilePathRole).toString();
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
    QFileSystemModel model;
    model.setRootPath(QDir::rootPath());
    fileBrowser.setModel(&model);
    fileBrowser.setRootIndex(model.index(QDir::rootPath()));
    fileBrowser.show();

    return app.exec();
}


