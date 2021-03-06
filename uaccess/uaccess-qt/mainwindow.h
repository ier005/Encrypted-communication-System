#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QStandardItemModel>
#include "optiondialog.h"
#include <QtDebug>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QFileDialog *fileDialog;
    int fd;
    QAction *icon_start, *icon_end;
    QAction *icon_import, *icon_export;
    QStandardItemModel *in_rules, *out_rules;
    OptionDialog *optionDialog;
    int in_id;  // save the present 'in' option id
    int out_id;  // save the present 'out' option id
    std::vector<QString> algs;  // save the crypto algrithms string

private slots:
    void on_icon_start_clicked();
    void on_icon_end_clicked();
    void on_icon_import_clicked();
    void on_icon_export_clicked();
    void on_out_add_clicked();
    void on_in_add_clicked();
    void on_out_mod_clicked();
    void on_in_mod_clicked();

    void on_tableView_clicked(const QModelIndex &index);

    void on_tableView_2_clicked(const QModelIndex &index);

    void on_out_del_clicked();

    void on_in_del_clicked();

public slots:
    void option_handle(int operation, int id, int alg, QString ip, QString key);

signals:
    void sig_option_info(int operation, int id, int alg, QString ip, QString key, int fd);
    void sig_original_option(int oper, int id, int type, QString ip, int key_len, QString key, int fd);
};

void cclose(int fd);

#endif // MAINWINDOW_H
