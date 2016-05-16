#ifndef DEVICE_TREE_WINDOW_H
#define DEVICE_TREE_WINDOW_H

#include <QMainWindow>
#include "fdt_model.h"

namespace Ui {
class Device_tree_window;
}

class Device_tree_window : public QMainWindow {
    Q_OBJECT

public:
    explicit Device_tree_window(QWidget *parent = 0);
    ~Device_tree_window();

private slots:
    void on_action_open_dtb_triggered();

    void on_actionQuit_triggered();

private:
    Ui::Device_tree_window *ui;
    fdt_model *fdt;
    QStringList file_args;
    void open_new_fdt_model(const QString &fdt_file);
    void add_edit_widgets(QModelIndex &dev_tree_index);
};

#endif // DEVICE_TREE_WINDOW_H
