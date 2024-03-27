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
    statusModel = new QStringListModel(this);
    ui->listView->setModel(statusModel);
}

MainWindow::~MainWindow()
{
    saveDirectories();
    delete ui;
}

auto MainWindow::genSimbolicToOldCharHhcCompatibility(QString realPath) -> QString {
    return realPath
        .replace("#", "[_SHARP_]")
        .replace("%", "[_PERCENT_]");
}

auto MainWindow::extractRealFromOldCharHhcCompatibility(QString simbolicPath) -> QString {
    return simbolicPath
        .replace("[_SHARP_]", "#")
        .replace("[_PERCENT_]", "%");
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
    if (exitCode != 1) {
         throw process.readAllStandardOutput();
    }
}

void MainWindow::createHhcFile(const QStringList& htmlFiles, const QString& hhcFilePath) {
    QFile hhcFile(hhcFilePath);
    if (hhcFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&hhcFile);
        out.setEncoding(QStringConverter::Latin1);
        out << "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">" << Qt::endl;
        out << "<HTML>" << Qt::endl;
        out << "<HEAD>" << Qt::endl;
        out << R"(<META name="GENERATOR" content="Microsoft&reg; HTML Help Workshop 4.1">)" << Qt::endl;
        out << "</HEAD>" << Qt::endl;
        out << "<BODY>" << Qt::endl;
        out << "<UL>" << Qt::endl;
        foreach(const QString& htmlFile, htmlFiles) {
            auto realHtmlFile = extractRealFromOldCharHhcCompatibility(changeFileExt(htmlFile, ""));
            out << "<LI><OBJECT type=\"text/sitemap\">" << Qt::endl;
            out << R"(<param name="Name" value=")" << realHtmlFile << "\">" << Qt::endl;
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
        stream.setEncoding(QStringConverter::Latin1);
        stream << "[OPTIONS]" << Qt::endl;
        stream << "Compatibility=1.1 or later" << Qt::endl;
        stream << "Compiled file=" << chmFilename << Qt::endl;
        stream << "Contents file=" << hhcFilename << Qt::endl;
        stream << "Default topic=" << htmlFiles.first() << Qt::endl;
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
    QString programFilesPath = qgetenv("ProgramFiles(x86)");
    QString cmd = programFilesPath + "/HTML Help Workshop/hhc.exe";
    executeHhpAndCapture(cmd, hhpFilePath);
}

auto MainWindow::changeFileExt(const QString& fileName, const QString& newExt) -> QString {
    QFileInfo fileInfo(fileName);
    return fileInfo.completeBaseName() + newExt;
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

void MainWindow::addStatus(const QString& status) {
    QStringList list = statusModel->stringList();
    list << status;
    statusModel->setStringList(list);
    QModelIndex index = statusModel->index(list.size() - 1);
    ui->listView->scrollTo(index);
}

void MainWindow::clearStatus() {
    statusModel->setStringList(QStringList());
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

    clearStatus();

    addStatus(QString("Criando a pasta de destino %1...").arg(outputDir));
    QDir().mkdir(outputDir);

    addStatus(QString("Diretório de trabalho: %1").arg(outputDir));
    QDir::setCurrent(outputDir);

    ui->pushButtonConvert->setEnabled(false);
    try {
        ui->progressBar->setMaximum(QDir(mdFilesDir).count() - 2);
        ui->progressBar->setValue(0);

        QDir mdDir(mdFilesDir);
        QStringList filters;
        filters << "*.md";
        QStringList mdFiles = mdDir.entryList(filters, QDir::Files);

        if (mdFiles.isEmpty()) {
            QMessageBox::critical(this, "Erro", "Nenhum arquivo MD encontrado na pasta de origem");
            return;
        }

        addStatus(QString("Arquivos MD encontrados: %1").arg(mdFiles.count()));

        foreach(const QString& mdFile, mdFiles) {
            QString htmlFile = changeFileExt(mdFile, ".html");
            convertMdToHtml(mdFilesDir + "/" + mdFile, outputDir + "/" + genSimbolicToOldCharHhcCompatibility(htmlFile));
            ui->statusbar->showMessage("Convertendo " + mdFile + " para " + htmlFile);
            addStatus(QString("Convertendo %1 para %2").arg(mdFile).arg(htmlFile));
            ui-> progressBar->setValue(ui->progressBar->value() + 1);
        }

        QDir htmlDir(outputDir);

        QStringList htmlFiles = htmlDir.entryList(QStringList() << "*.html", QDir::Files);

        if (htmlFiles.isEmpty()) {
            QMessageBox::critical(this, "Erro", "Nenhum arquivo HTML gerado!");
            return;
        }

        auto hhcFilename = QString("%1.%2").arg(helpName, "hhc");
        auto hhpFilename = QString("%1.%2").arg(helpName, "hhp");
        auto chmFilename = QString("%1.%2").arg(helpName, "chm");

        addStatus(QString("Gerando arquivo HHC: %1").arg(hhcFilename));
        createHhcFile(htmlFiles, hhcFilename);

        addStatus(QString("Gerando arquivo HHP: %1").arg(hhpFilename));
        createHhpFile(htmlFiles, hhpFilename);

        addStatus("Compilando CHM...");
        compileChm(hhpFilename);

        if (!QFile(chmFilename).exists()) {
            throw std::runtime_error("Erro ao compilar CHM");
        }

        ui->statusbar->showMessage("Arquivo CHM gerado com sucesso: " + chmFilename);
        saveDirectories();
        addStatus("Arquivo CHM gerado com sucesso: " + chmFilename);
        QMessageBox::information(this, "Sucesso", "Arquivo CHM gerado com sucesso: " + chmFilename);
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

