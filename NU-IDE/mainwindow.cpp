#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QStringList>
#include <QFileDialog>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QInputDialog>
#include <QStandardPaths>
#include <QSerialPort>
#include <QByteArray>

#include "highlighting.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    serial(new QSerialPort(this))
{
    ui->setupUi(this);

    // --- Populate Serial Ports ---
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        ui->Port->addItem(port.systemLocation());
    }
    CppHighlighter *highlighter = new CppHighlighter(ui->CodeT->document());
    if (ui->Port->count() == 0) {
        ui->Port->addItem("No ports available");
    }

    // --- Boards ---
    ui->Board->addItem("Arduino Uno", "arduino:avr:uno");
    ui->Board->addItem("Arduino Nano", "arduino:avr:nano");
    ui->Board->addItem("Arduino Mega 2560", "arduino:avr:mega");
    ui->Board->addItem("Arduino Leonardo", "arduino:avr:leonardo");
    ui->Board->addItem("Arduino Micro", "arduino:avr:micro");
    ui->Board->addItem("Arduino Pro Mini 5V/16MHz", "arduino:avr:pro:cpu=16MHzatmega328");
    ui->Board->addItem("Arduino Pro Mini 3.3V/8MHz", "arduino:avr:pro:cpu=8MHzatmega328");

    // --- Check if arduino-cli exists ---

    if (QStandardPaths::findExecutable("arduino-cli.exe").isEmpty()) {
        QMessageBox::critical(this, "Error", "arduino-cli not found in PATH.");
        return;
    }

    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);

}

MainWindow::~MainWindow()
{
    delete ui;
}

// ===================== CREATE =====================
void MainWindow::on_actionCreate_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Create Arduino Sketch",
        "",
        "Arduino Files (*.ino)"
        );

    if (fileName.isEmpty())
        return;

    if (!fileName.endsWith(".ino"))
        fileName += ".ino";

    QFile file(fileName);

    QString templateCode = R"(void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
})";

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&file);
        ts << templateCode;
        file.close();

        currentFilePath = fileName;
        ui->CodeT->setPlainText(templateCode);
    } else {
        QMessageBox::warning(this, "Error", file.errorString());
    }
}

// ===================== OPEN =====================
void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open File",
        "",
        "Arduino Files (*.ino);;All Files (*)"
        );

    if (fileName.isEmpty())
        return;

    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to open file.");
        return;
    }

    QTextStream in(&file);
    ui->CodeT->setPlainText(in.readAll());
    file.close();

    currentFilePath = fileName;
}

// ===================== SAVE =====================
void MainWindow::on_actionClose_save_triggered()
{
    if (currentFilePath.isEmpty()) {
        currentFilePath = QFileDialog::getSaveFileName(
            this,
            "Save File",
            "",
            "Arduino Files (*.ino)"
            );
    }

    if (currentFilePath.isEmpty())
        return;

    QFile file(currentFilePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to save file.");
        return;
    }

    QTextStream out(&file);
    out << ui->CodeT->toPlainText();
    file.close();
}

// ===================== BUILD TEMP SKETCH =====================
QString MainWindow::prepareSketchFolder()
{
    if (currentFilePath.isEmpty())
        return "";

    QFileInfo fileInfo(currentFilePath);

    if (fileInfo.suffix() != "ino")
        return "";

    QTemporaryDir tempDir;
    tempDir.setAutoRemove(false);

    if (!tempDir.isValid())
        return "";

    QString folderPath = tempDir.path();


    QString folderName = QFileInfo(folderPath).fileName();


    QString newFilePath = folderPath + "/" + folderName + ".ino";

    if (!QFile::copy(currentFilePath, newFilePath)) {
        return "";
    }

    return folderPath;
}

// ===================== UPLOAD =====================
void MainWindow::on_actionUpload_triggered()
{
    if (currentFilePath.isEmpty()) {
        QMessageBox::warning(this, "Error", "No file opened.");
        return;
    }

    // Save latest edits
    on_actionClose_save_triggered();

    QString sketchDir = prepareSketchFolder();

    if (sketchDir.isEmpty()) {
        QMessageBox::warning(this, "Error", "Invalid sketch.");
        return;
    }

    QString program = "arduino-cli";

    // ---------- COMPILE ----------
    QProcess compileProcess;
    QStringList compileArgs;

    compileArgs << "compile"
                << "--fqbn" << ui->Board->currentData().toString()
                << sketchDir;

    compileProcess.start(program, compileArgs);

    if (!compileProcess.waitForFinished(-1)) {
        QMessageBox::critical(this, "Error", "Compile failed to start.");
        return;
    }

    QString compileError = compileProcess.readAllStandardError();

    if (!compileError.isEmpty()) {
        QMessageBox::critical(this, "Compile Error", compileError);
        return;
    }

    // ---------- UPLOAD ----------
    QProcess uploadProcess;
    QStringList uploadArgs;

    uploadArgs << "upload"
               << "-p" << ui->Port->currentText()
               << "--fqbn" << ui->Board->currentData().toString()
               << sketchDir;

    // These arguments are needed for older bootloader clones.

    if (ui->oldBootloaderCheckBox->isChecked()) {
        uploadArgs << "--protocol" << "stk500v1";
    }

    uploadProcess.start(program, uploadArgs);

    if (!uploadProcess.waitForFinished(-1)) {
        QMessageBox::critical(this, "Error", "Upload failed to start.");
        return;
    }

    QString uploadError = uploadProcess.readAllStandardError();

    if (!uploadError.isEmpty()) {
        QMessageBox::critical(this, "Upload Error", uploadError);
        return;
    }

    QMessageBox::information(this, "Success", "Upload completed!");
}

// ===================== INSTALL LIB =====================

void MainWindow::on_actionAdd_triggered()
{
    bool ok;
    QString lib = QInputDialog::getText(this, "Lib manager", "Lib name:",
                                        QLineEdit::Normal, "", &ok);

    if (ok && !lib.isEmpty()) {

        QProcess process;

        QString program = "arduino-cli";
        QStringList args;
        args << "lib" << "install" << lib;

        process.start(program, args);
        process.waitForFinished(-1);

        QString output = process.readAllStandardOutput();
        QString error  = process.readAllStandardError();

        if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
            QMessageBox::information(this, "Lib manager",
                                     "Library installed successfully:\n" + lib +
                                         "\n\n" + output);
        } else {
            QMessageBox::critical(this, "Lib manager",
                                  "Failed to install library:\n" + lib +
                                      "\n\nError:\n" + error);
        }
    }
}

// ===================== REMOVE LIB =====================

void MainWindow::on_actionRemove_Library_triggered()
{
    bool ok;
    QString lib = QInputDialog::getText(this, "Lib manager", "Lib name:",
                                        QLineEdit::Normal, "", &ok);

    if (ok && !lib.isEmpty()) {

        QProcess process;

        QString program = "arduino-cli";
        QStringList args;
        args << "lib" << "uninstall" << lib;

        process.start(program, args);
        process.waitForFinished(-1);

        QString output = process.readAllStandardOutput();
        QString error  = process.readAllStandardError();

        if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
            QMessageBox::information(this, "Lib manager",
                                     "Library removed successfully:\n" + lib +
                                         "\n\n" + output);
        } else {
            QMessageBox::critical(this, "Lib manager",
                                  "Failed to remove library:\n" + lib +
                                      "\n\nError:\n" + error);
        }
    }
}
void MainWindow::on_actionAdd_lib_from_zip_file_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Library ZIP",
        "",
        "ZIP Files (*.zip)"
        );

    if (filePath.isEmpty())
        return;

    QProcess process;

    QStringList args;
    args << "lib" << "install" << "--zip-path" << filePath;

    process.start("arduino-cli", args);
    process.waitForFinished(-1);

    QString output = process.readAllStandardOutput();
    QString error  = process.readAllStandardError();

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        QMessageBox::information(this, "Lib manager",
                                 "Library installed from ZIP successfully:\n" +
                                     filePath + "\n\n" + output);
    } else {
        QMessageBox::critical(this, "Lib manager",
                              "Failed to install library from ZIP:\n" +
                                  filePath + "\n\nError:\n" + error);
    }
}


// ===================== CONNECT =====================
void MainWindow::on_Connect_clicked()
{
    if (serial->isOpen()) {
        qDebug() << "Already connected.";
        return;
    }

    // Get baud rate
    bool ok;
    int baudRate = ui->BaudRate->text().toInt(&ok);
    if (!ok || baudRate <= 0) {
        QMessageBox::warning(this, "Error", "Invalid baud rate entered!");
        return;
    }

    QString portName = ui->Port->currentText();
    if (portName.isEmpty() || portName == "No ports available") {
        QMessageBox::warning(this, "Error", "No valid serial port selected.");
        return;
    }

    serial->setPortName(portName);
    serial->setBaudRate(baudRate);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (serial->open(QIODevice::ReadWrite)) {
        serialBuffer.clear(); // start fresh
        qDebug() << "Serial port connected at" << baudRate << "baud.";

        // Connect signals
        connect(serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
        connect(serial, &QSerialPort::errorOccurred, this, &MainWindow::handleSerialError);

        ui->SerialText->appendPlainText("Connected to " + portName + " at " + QString::number(baudRate) + " baud.");
    } else {
        QMessageBox::critical(this, "Error", "Failed to open port:\n" + serial->errorString());
    }
}

// ===================== DISCONNECT =====================
void MainWindow::on_Disconnect_clicked()
{
    if (serial->isOpen()) {
        serial->close();
        serialBuffer.clear();
        ui->SerialText->appendPlainText("Serial port disconnected.");
        qDebug() << "Serial port disconnected.";
    }
}

// ===================== READ SERIAL =====================
void MainWindow::readSerialData()
{
    serialBuffer += serial->readAll();

    int index;
    while ((index = serialBuffer.indexOf('\n')) != -1) {
        QByteArray line = serialBuffer.left(index + 1);
        serialBuffer.remove(0, index + 1);

        ui->SerialText->appendPlainText(QString::fromUtf8(line).trimmed());
        ui->SerialText->centerOnScroll();
    }
}

// ===================== SERIAL ERROR =====================
void MainWindow::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        QMessageBox::warning(this, "Serial Error", serial->errorString());
        qDebug() << "Serial error:" << serial->errorString();
    }
}