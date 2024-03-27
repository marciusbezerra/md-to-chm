#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

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
    void convertMdToHtml(const QString& mdFilePath, const QString& htmlFilePath);
    void saveDirectories();
    void loadDirectories();
    Ui::MainWindow *ui;
    void createHhpFile(const QStringList &htmlFiles, const QString &hhpFilePath);
    void compileChm(const QString &hhpFilePath);
    void executeHhpAndCapture(const QString &cmd, const QString &args);
    auto changeFileExt(const QString &fileName, const QString &newExt) -> QString;
};
#endif // MAINWINDOW_H
