#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QSettings>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    loadDirectories();
    ui->progressBar->setValue(0);
}

MainWindow::~MainWindow()
{
    saveDirectories();
    delete ui;
}

void MainWindow::convertMdToHtml(const QString& mdFilePath, const QString& htmlFilePath) {
    QTextDocument doc;
    QFile file(mdFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Erro", "Erro ao abrir arquivo MD: " + mdFilePath);
        return;
    }
    QByteArray document = file.readAll();
    doc.setMarkdown(document);
    QString html = doc.toHtml();
    QFile htmlFile(htmlFilePath);
    if (htmlFile.open(QIODevice::WriteOnly)) {
        QTextStream stream(&htmlFile);
        stream << html;
        htmlFile.close();
    }
    // QString cmd = "C:/Program Files (x86)/HTML Help Workshop/markdown2html " + mdFilePath + " > " + htmlFilePath;
    // QProcess::execute(cmd);
}

void MainWindow::executeHhpAndCapture(const QString& cmd, const QString& args) {
    QProcess process;
    process.start(cmd, QStringList() << args);
    process.waitForFinished();
    auto output = process.readAllStandardOutput();
    auto error = process.readAllStandardError();
    auto exitCode = process.exitCode();
    qDebug() << "Output: " << output;
    qDebug() << "Error: " << error;
    qDebug() << "Exit code: " << exitCode;
    // if ( != 0) {
    //     throw process.readAllStandardOutput();
    // }
}

void createHhcFile(const QStringList& htmlFiles, const QString& hhcFilePath) {
    QFile hhcFile(hhcFilePath);
    if (hhcFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&hhcFile);
        out << "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">" << Qt::endl;
        out << "<HTML>" << Qt::endl;
        out << "<HEAD>" << Qt::endl;
        out << "<META name=\"GENERATOR\" content=\"Microsoft&reg; HTML Help Workshop 4.1\">" << Qt::endl;
        out << "</HEAD>" << Qt::endl;
        out << "<BODY>" << Qt::endl;
        out << "<UL>" << Qt::endl;
        foreach(const QString& htmlFile, htmlFiles) {
            out << "<LI><OBJECT type=\"text/sitemap\">" << Qt::endl;
            out << R"(<param name="Name" value=")" << htmlFile << "\">" << Qt::endl;
            out << R"(<param name="Local" value=")" << htmlFile << "\">" << Qt::endl;
            out << "</OBJECT>" << Qt::endl;
        }
        out << "</UL>" << Qt::endl;
        out << "</BODY>" << Qt::endl;
        out << "</HTML>" << Qt::endl;
        hhcFile.close();
    } else {
        throw "Erro", "Erro ao criar arquivo HHC: " + hhcFilePath;
    }
}

void MainWindow::createHhpFile(const QStringList& htmlFiles, const QString& hhpFilename) {

    auto chmFilename = changeFileExt(hhpFilename, ".chm");
    auto hhcFilename = changeFileExt(hhpFilename, ".hhc");
    auto helpTitle = changeFileExt(hhpFilename, "");

    QFile hhpFile(hhpFilename);
    if (hhpFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&hhpFile);
        stream.setEncoding(QStringConverter::Utf8);
        stream << "[OPTIONS]" << Qt::endl;
        stream << "Compatibility=1.1 or later" << Qt::endl;
        stream << "Compiled file=" << chmFilename << Qt::endl;
        stream << "Contents file=" << hhcFilename << Qt::endl;
        //stream << "Default topic=index.html" << Qt::endl;
        stream << "Display compile progress=No" << Qt::endl;
        stream << "Full-text search=Yes" << Qt::endl;
        stream << "Language=0x416 Portuguese (Brazil)" << Qt::endl;
        stream << "Title=" << helpTitle << Qt::endl;
        stream << "[FILES]" << Qt::endl;
        foreach(const QString& htmlFile, htmlFiles) {
            stream << htmlFile << Qt::endl;
        }
        hhpFile.close();
    }
}

void MainWindow::compileChm(const QString& hhpFilePath) {
    QString cmd = "C:/Program Files (x86)/HTML Help Workshop/hhc.exe";
    executeHhpAndCapture(cmd, hhpFilePath);
}

auto MainWindow::changeFileExt(const QString& fileName, const QString& newExt) -> QString {
    QFileInfo fileInfo(fileName);
    return fileInfo.baseName() + newExt;
}

void MainWindow::saveDirectories() {
    QString mdFilesDir = ui->lineEditSource->text();
    QString htmlFilesDir = ui->lineEditDest->text();
    QSettings settings("MCB", QApplication::applicationName());
    settings.setValue("mdFilesDir", mdFilesDir);
    settings.setValue("htmlFilesDir", htmlFilesDir);
}

void MainWindow::loadDirectories() {
    QSettings settings("MCB", QApplication::applicationName());
    QString mdFilesDir = settings.value("mdFilesDir").toString();
    QString htmlFilesDir = settings.value("htmlFilesDir").toString();
    ui->lineEditSource->setText(mdFilesDir);
    ui->lineEditDest->setText(htmlFilesDir);
}

void MainWindow::on_pushButtonConvert_clicked()
{
    QString mdFilesDir = ui->lineEditSource->text(); // Pasta com os arquivos MD
    QString outputDir = ui->lineEditDest->text(); // Pasta com os arquivos HTML
    QString helpName = ui->lineEditHelpName->text(); // Nome do arquivo de ajuda

    if (mdFilesDir.isEmpty() || outputDir.isEmpty() || helpName.isEmpty()) {
        QMessageBox::critical(this, "Erro", "Preencha todos os campos!");
        return;
    }

    if (!QDir(mdFilesDir).exists()) {
        return;
    }

    if (QDir(outputDir).exists()) {
        if (QMessageBox::question(this, "Pasta de destino já existe", "Deseja apagar a pasta de destino e continuar?") == QMessageBox::No) {
            return;
        }
        QDir(outputDir).removeRecursively();
    }

    QDir().mkdir(outputDir);

    QDir::setCurrent(outputDir);

    ui->pushButtonConvert->setEnabled(false);
    try {
        ui->progressBar->setMaximum(QDir(mdFilesDir).count() - 2);
        ui->progressBar->setValue(0);

        QDir mdDir(mdFilesDir);
        QStringList filters;
        filters << "*.md";
        QStringList mdFiles = mdDir.entryList(filters, QDir::Files);
        foreach(const QString& mdFile, mdFiles) {
            QString htmlFile = changeFileExt(mdFile, ".html");
            convertMdToHtml(mdFilesDir + "/" + mdFile, outputDir + "/" + htmlFile);
            ui->statusbar->showMessage("Convertendo " + mdFile + " para " + htmlFile);
            ui-> progressBar->setValue(ui->progressBar->value() + 1);
        }

        QDir htmlDir(outputDir);

        QStringList htmlFiles = htmlDir.entryList(QStringList() << "*.html", QDir::Files);

        auto hhcFilename = QString("%1.%2").arg(helpName, "hhc");
        auto hhpFilename = QString("%1.%2").arg(helpName, "hhp");
        auto chmFilename = QString("%1.%2").arg(helpName, "chm");

        createHhcFile(htmlFiles, hhcFilename);
        createHhpFile(htmlFiles, hhpFilename);
        compileChm(hhpFilename);

        if (!QFile(chmFilename).exists()) {
            throw std::runtime_error("Erro ao compilar CHM");
        }

        ui->statusbar->showMessage("Arquivo CHM gerado com sucesso: " + chmFilename);
        saveDirectories();
        QMessageBox::information(this, "Sucesso", "Arquivo CHM gerado com sucesso: " + chmFilename);
        // open chm file using default application
        QDesktopServices::openUrl(QUrl::fromLocalFile(chmFilename));

    } catch (std::exception& e) {
        qDebug() << "Erro ao converter arquivos MD para HTML: " << e.what();
        QMessageBox::critical(this, "Erro", "Erro ao converter arquivos MD para HTML");
    }
    ui->pushButtonConvert->setEnabled(true);
}


void MainWindow::on_toolButtonSelectDirSource_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Selecione a pasta com os arquivos MD");
    if (!dir.isEmpty()) {
        ui->lineEditSource->setText(dir);
    }
}


void MainWindow::on_toolButtonSelectDirDest_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Selecione a pasta de destino (com os arquivos HTML)");
    if (!dir.isEmpty()) {
        ui->lineEditDest->setText(dir);
    }
}

