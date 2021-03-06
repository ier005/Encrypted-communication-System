#include "mainwindow.h"
#include "ui_mainwindow.h"

void cclose(int fd)
{
    close(fd);
}

// in the constructor function we initial some variables,
// open the virtual driver file to deliver the user-set options
// and insert the kernel module (the path is fixed!)
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    fileDialog(new QFileDialog),
    icon_start(new QAction(QIcon(":/start.png"), tr("Start the encrypted system"), this)),
    icon_end(new QAction(QIcon(":/end.png"), tr("Stop the encrypted system"), this)),
    icon_import(new QAction(QIcon(":/import.png"), tr("Import rules"), this)),
    icon_export(new QAction(QIcon(":/export.png"), tr("Export rules"), this)),
    in_rules(new QStandardItemModel),
    out_rules(new QStandardItemModel),
    optionDialog(new OptionDialog(this)),
    out_id(0),
    in_id(0)
{
    ui->setupUi(this);

    algs.push_back(tr("AES-ECB(128)"));
    algs.push_back(tr("AES-CBC(128)"));
    algs.push_back(tr("AES-XTS(128)"));
    algs.push_back(tr("AES-CTR(128)"));

    icon_end->setEnabled(0);
    ui->mainToolBar->addAction(icon_start);
    ui->mainToolBar->addAction(icon_end);
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(icon_import);
    ui->mainToolBar->addAction(icon_export);

    in_rules->setHorizontalHeaderItem(0, new QStandardItem(tr("ID")));
    in_rules->setHorizontalHeaderItem(1, new QStandardItem(tr("Cipher alg")));
    in_rules->setHorizontalHeaderItem(2, new QStandardItem(tr("IP")));
    in_rules->setHorizontalHeaderItem(3, new QStandardItem(tr("key")));

    out_rules->setHorizontalHeaderItem(0, new QStandardItem(tr("ID")));
    out_rules->setHorizontalHeaderItem(1, new QStandardItem(tr("Cipher alg")));
    out_rules->setHorizontalHeaderItem(2, new QStandardItem(tr("IP")));
    out_rules->setHorizontalHeaderItem(3, new QStandardItem(tr("key")));

    ui->tableView->setModel(out_rules);
    ui->tableView->verticalHeader()->hide();
    ui->tableView->setColumnWidth(0, 40);
    ui->tableView->setColumnWidth(1, 160);
    ui->tableView->setColumnWidth(2, 160);
    ui->tableView->setColumnWidth(3, 200);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->tableView_2->setModel(in_rules);
    ui->tableView_2->verticalHeader()->hide();
    ui->tableView_2->setColumnWidth(0, 40);
    ui->tableView_2->setColumnWidth(1, 160);
    ui->tableView_2->setColumnWidth(2, 160);
    ui->tableView_2->setColumnWidth(3, 200);
    ui->tableView_2->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_2->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->out_mod->setEnabled(0);
    ui->in_mod->setEnabled(0);
    ui->out_del->setEnabled(0);
    ui->in_del->setEnabled(0);

    if ((fd = open("/dev/enccom", O_WRONLY, S_IWUSR)) == -1) {
        QMessageBox::critical(this, "Error", "Can not open /dev/enccom!");
        ui->out_add->setEnabled(0);
        ui->in_add->setEnabled(0);
        icon_start->setEnabled(0);
        icon_import->setEnabled(0);
    }

    ui->statusBar->showMessage(tr("The encrypted system has been stopped."));

    connect(icon_start, &QAction::triggered, this, &MainWindow::on_icon_start_clicked);
    connect(icon_end, &QAction::triggered, this, &MainWindow::on_icon_end_clicked);
    connect(icon_import, &QAction::triggered, this, &MainWindow::on_icon_import_clicked);
    connect(icon_export, &QAction::triggered, this, &MainWindow::on_icon_export_clicked);

    connect(optionDialog, &OptionDialog::sig_option, this, &MainWindow::option_handle);
    connect(this, &MainWindow::sig_option_info, optionDialog, &OptionDialog::option_info);
    connect(this, &MainWindow::sig_original_option, optionDialog, &OptionDialog::handle_original_option);
}

// in the destructor we free some variables , close the virtual driver file and remove the kernel module
MainWindow::~MainWindow()
{
    delete in_rules;
    delete out_rules;
    delete icon_start;
    delete icon_end;
    delete icon_import;
    delete icon_export;
    delete fileDialog;
    delete ui;
    cclose(fd);
    if (system("rmmod enccom"))
        QMessageBox::critical(this, "Error", "Can not remove the encrypted module!");
}

// set the mod_running variable in the kernel module to make the module in use
void MainWindow::on_icon_start_clicked()
{
    unsigned char opt = 0;
    write(fd, &opt, 1);
    icon_start->setEnabled(0);
    icon_end->setEnabled(1);
    ui->statusBar->showMessage(tr("The encrypted system is running!"));
}

// unset the mod_running variable
void MainWindow::on_icon_end_clicked()
{
    unsigned char opt = 0;
    write(fd, &opt, 1);
    icon_start->setEnabled(1);
    icon_end->setEnabled(0);
    ui->statusBar->showMessage(tr("The encrypted system has been stopped."));
}

// open an text file and analyse the option
// line begin with '#' is treated as comment
// emit a signal to call the funtion of OptionDialog to add options
// Warning: Not check the input validation yet
void MainWindow::on_icon_import_clicked()
{
    QString filename = fileDialog->getOpenFileName(this, "Open");
    if (filename.isEmpty())
        return;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Can not open the file");
        return;
    }
    QTextStream out(&file);
    while (!out.atEnd()) {
        QString sio, salg, ip, key;
        out >> sio;
        if (sio.isEmpty()) {
            out.skipWhiteSpace();
            continue;
        }
        if (sio.startsWith('#')) {
            out.readLine();
            continue;
        }

        out >> salg >> ip >> key;
        int oper = (sio == "out") ? 0 : 1;
        int id = (sio == "out") ? this->out_id : this->in_id;
        int alg = 0;
        while (salg != algs.at(alg))
            alg++;

        emit sig_original_option(oper, id, alg, ip, key.length(), key, this->fd);
    }
}

// choose a path and save the present options into a text file
void MainWindow::on_icon_export_clicked()
{
    QString filename = fileDialog->getSaveFileName(this, "Save as", "./rules.txt");
    if (filename.isEmpty())
        return;
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Can not open the file!");
        return;
    }
    QTextStream in(&file);
    int rows = out_rules->rowCount();
    for (int i = 0; i < rows; i++) {
        QString s("out ");
        QModelIndex index = out_rules->index(i, 1);
        s += out_rules->data(index).toString();
        s += " ";
        index = out_rules->index(i, 2);
        s += out_rules->data(index).toString();
        s += " ";
        index = out_rules->index(i, 3);
        s += out_rules->data(index).toString();
        s += "\n";
        in << s;
    }

    rows = in_rules->rowCount();
    for (int i = 0; i < rows; i++) {
        QString s("in ");
        QModelIndex index = in_rules->index(i, 1);
        s += in_rules->data(index).toString();
        s += " ";
        index = in_rules->index(i, 2);
        s += in_rules->data(index).toString();
        s += " ";
        index = in_rules->index(i, 3);
        s += in_rules->data(index).toString();
        s += "\n";
        in << s;
    }
    file.close();

}


// add or modify the options of out and in chains in kernel module,
// it opens a dialog and user can set the parameter
// when adding we should deliver the next option id.
// when modifying we should let the dialog konw which option
// is to be modified and its present options
void MainWindow::on_out_add_clicked()
{
    emit sig_option_info(0, out_id, 0, tr(""), tr(""), fd);
    optionDialog->exec();
}

void MainWindow::on_in_add_clicked()
{
    emit sig_option_info(1, in_id, 0, tr(""), tr(""), fd);
    optionDialog->exec();
}

void MainWindow::on_out_mod_clicked()
{
    QModelIndex index = this->ui->tableView->currentIndex();
    int row = index.row();

    index = out_rules->index(row, 0);
    QString sid = out_rules->data(index).toString();
    int id = sid.toInt();

    index = out_rules->index(row, 1);
    QString salg = out_rules->data(index).toString();

    int alg = 0;
    while (salg != algs.at(alg))
        alg++;

    index = out_rules->index(row, 2);
    QString ip = out_rules->data(index).toString();

    index = out_rules->index(row, 3);
    QString key = out_rules->data(index).toString();

    emit sig_option_info(2, id, alg, ip, key, fd);

    optionDialog->exec();
}

void MainWindow::on_in_mod_clicked()
{
    QModelIndex index = this->ui->tableView_2->currentIndex();
    int row = index.row();

    index = in_rules->index(row, 0);
    QString sid = in_rules->data(index).toString();
    int id = sid.toInt();

    index = in_rules->index(row, 1);
    QString salg = in_rules->data(index).toString();
    int alg = 0;
    while (salg != algs.at(alg))
        alg++;

    index = in_rules->index(row, 2);
    QString ip = in_rules->data(index).toString();

    index = in_rules->index(row, 3);
    QString key = in_rules->data(index).toString();

    emit sig_option_info(3, id, alg, ip, key, fd);
    optionDialog->exec();
}


// the option-set dialog ddliver the new option patameters, so that we can
// show them in tables
void MainWindow::option_handle(int operation, int id, int alg, QString ip, QString key)
{
    QString salg = algs.at(alg);
    if (operation < 2) {
        QList<QStandardItem *> list;
        list.append(new QStandardItem(QString::number(id, 10)));
        list.append(new QStandardItem(salg));
        list.append(new QStandardItem(ip));
        list.append(new QStandardItem(key));
        if (operation == 0) {
            out_rules->appendRow(list);
            out_id = id + 1;
        } else {
            in_rules->appendRow(list);
            in_id = id + 1;
        }
    } else {
        QStandardItemModel *rules = (operation == 2) ? out_rules : in_rules;
        int rows = rules->rowCount();
        QModelIndex index;
        for (int i = 0; i < rows; i++) {
            index = rules->index(i, 0);
            if (id == rules->data(index).toInt()) {
                rules->setItem(i, 1, new QStandardItem(algs[alg]));
                rules->setItem(i, 2, new QStandardItem(ip));
                rules->setItem(i, 3, new QStandardItem(key));
                break;
            }
        }
    }

    this->ui->in_mod->setEnabled(0);
    this->ui->in_del->setEnabled(0);
    this->ui->out_mod->setEnabled(0);
    this->ui->out_del->setEnabled(0);
}

void MainWindow::on_tableView_clicked(const QModelIndex &index)
{
    this->ui->out_mod->setEnabled(1);
    this->ui->out_del->setEnabled(1);
}

void MainWindow::on_tableView_2_clicked(const QModelIndex &index)
{
    this->ui->in_mod->setEnabled(1);
    this->ui->in_del->setEnabled(1);
}


// deleting option is done in mainwindow directly
void MainWindow::on_out_del_clicked()
{
    QModelIndex index = ui->tableView->currentIndex();
    int id = out_rules->data(index).toInt();
    char opt[5];
    opt[0] = 3;
    opt[1] = 1;
    memcpy(opt + 2, &id, 4);
    write(fd, opt, 6);

    out_rules->removeRow(index.row());
    if (out_rules->rowCount() == 0) {
        ui->out_mod->setEnabled(0);
        ui->out_del->setEnabled(0);
    }

    this->ui->out_mod->setEnabled(1);
    this->ui->out_del->setEnabled(1);
}

void MainWindow::on_in_del_clicked()
{
    QModelIndex index = ui->tableView_2->currentIndex();
    int id = in_rules->data(index).toInt();
    char opt[5];
    opt[0] = 3;
    opt[1] = 0;
    memcpy(opt + 2, &id, 4);
    write(fd, opt, 6);

    in_rules->removeRow(index.row());
    if (in_rules->rowCount() == 0) {
        ui->in_mod->setEnabled(0);
        ui->in_del->setEnabled(0);
    }

    this->ui->in_mod->setEnabled(1);
    this->ui->in_del->setEnabled(1);
}
