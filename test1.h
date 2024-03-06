#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_test1.h"

class test1 : public QMainWindow
{
    Q_OBJECT

public:
    test1(QWidget *parent = nullptr);
    ~test1();

private slots:

private:
    Ui::test1Class ui;
};
