#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QModelIndex>
#include <QScrollArea>
#include <QLabel>

#include "device_tree_window.h"
#include "ui_device_tree_window.h"
#include "fdt_model.h"
#include <sys/mman.h>


Device_tree_window::Device_tree_window(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Device_tree_window) {
    ui->setupUi(this);
    file_args = QCoreApplication::arguments();
    if (file_args.count() > 1) {
        open_new_fdt_model(file_args[1]);
    }
}

Device_tree_window::~Device_tree_window() {
    delete ui;
}

void Device_tree_window::add_edit_widgets(QModelIndex &dev_tree_index) {
    QModelIndex property_value_index;
    QModelIndex property_name_index;
    int rows = fdt->rowCount(dev_tree_index);
    for (int i=0; i < rows; ++i) {
        property_value_index = fdt->index(i, 1, dev_tree_index);
        property_name_index = fdt->index(i, 0, dev_tree_index);
        int item_rows = fdt->rowCount(property_name_index
);
        qDebug() << "item = " << property_value_index << "item_rows = " << item_rows;
        if (fdt->hasChildren(property_name_index)) {
            add_edit_widgets(property_name_index);
        } else {
            QScrollArea *property_editor;
            QLabel *prop_string_label;
            QVariant property_data = fdt->data(property_value_index, Qt::DisplayRole);
            QStringList prop_strings = property_data.toStringList();
            QString prop_string;
            QPalette::ColorRole name_background;
            if ((i & 0x1) == 1) {
                name_background = QPalette::Base;
            } else {
                name_background = QPalette::AlternateBase;
            }
            if (prop_strings.count() > 0) {
                prop_string = prop_strings[0];
            } // endif
            property_editor = new QScrollArea();
            property_editor->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
            property_editor->setBackgroundRole(name_background);
            prop_string_label = new QLabel(prop_string, property_editor);
            property_editor->setWidgetResizable(true);
            property_editor->setWidget(prop_string_label);
            prop_string_label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
            prop_string_label->setWordWrap(true);
            ui->device_tree->setIndexWidget(property_value_index, property_editor);
        } // endif
    } // endfor

}
void Device_tree_window::open_new_fdt_model(const QString &fdt_file) {
    QModelIndex dev_tree_index;
    if (fdt_file.isEmpty()) {
        fdt = new fdt_model();
    } else {
        fdt = new fdt_model(fdt_file);
    }
    ui->device_tree->setModel(fdt);
    dev_tree_index = fdt->index(0, 0, dev_tree_index);
    ui->device_tree->resizeColumnToContents(0);
    ui->device_tree->expand(dev_tree_index);  // Unfold the "/" element of the device tree.
    add_edit_widgets(dev_tree_index);
    ui->actionClose->setEnabled(true);

}

void Device_tree_window::on_action_open_dtb_triggered() {
    open_new_fdt_model(QString());
}

void Device_tree_window::on_actionQuit_triggered() {
    QApplication::exit(0);
}
