#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>

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
    ~MainWindow();

private slots:
    void on_actionCreate_triggered();

    void on_actionUpload_triggered();

    void on_actionOpen_triggered();

    void on_actionClose_save_triggered();

    QString prepareSketchFolder();

    void on_actionAdd_triggered();

    void on_actionRemove_Library_triggered();

    void on_actionAdd_lib_from_zip_file_triggered();

    void on_Connect_clicked();

    void on_Disconnect_clicked();

    void readSerialData();

    void handleSerialError(QSerialPort::SerialPortError error);

private:
    Ui::MainWindow *ui;
    QString currentFilePath;
    QSerialPort *serial;
    QByteArray serialBuffer;
};
#endif // MAINWINDOW_H
