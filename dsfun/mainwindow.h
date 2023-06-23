#pragma once

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget.h>
#include <QStringListModel.h>
#include <QStringList.h>
#include <QMessagebox.h>
#include <QThread.h>
#include <IXHttpClient.h>
#include <IXWebSocket.h>
#include <string.h>
#include <sstream>
#include "timedRead.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow() override;

protected:
    Ui::MainWindow* ui;
    std::shared_ptr<dsiisotfsReroute> gameInstance = std::make_shared<dsiisotfsReroute>();

private:
    QStringListModel* model;
    std::shared_ptr<timedRead> timer;
    int runFlag = 0;
    int attachFlag = 0;
    int injectFlag = 0;

    std::vector<std::string> splitString(const std::string& str);

private slots:
    void on_scanButton_clicked();
    void on_goButton_clicked();
    void on_attachButton_clicked();
};

#endif