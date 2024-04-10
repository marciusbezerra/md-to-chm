#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class Node {
public:
    QString title;
    QString path;
    QList<Node> children;

    Node(const QString& title, const QString& path) : title(title), path(path) {}

    void addChild(const Node& child) {
        children.append(child);
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_pushButtonConvert_clicked();
    void on_toolButtonSelectDirSource_clicked();
    void on_toolButtonSelectDirDest_clicked();
private:
    QStringListModel *statusModel;

    void convertMdToHtml(const QString& mdFilePath, const QString& htmlFilePath);
    void saveDirectories();
    void loadDirectories();
    Ui::MainWindow *ui;
    void createHhpFile(const QStringList &htmlFiles, const QString &hhpFilePath);
    void compileChm(const QString &hhpFilePath);
    void executeHhpAndCapture(const QString &cmd, const QString &args);
    auto changeFileExt(const QString &fileName, const QString &newExt) -> QString;
    void createHhcFile(const Node root, const QString &hhcFilePath);
    void addStatus(const QString &status);
    void clearStatus();
    QString genSimbolicToOldCharHhcCompatibility(QString realPath);
    QString extractRealFromOldCharHhcCompatibility(QString simbolicPath);
    void convertMdToHtmlRecursively(const QString &mdFilesDir, const QString &outputDir, QStringList &htmlFiles);
    Node buildTree(const QStringList &files);
    void addHhcObject(Node node, QTextStream &out);
};
#endif // MAINWINDOW_H
