#include <QWidget>
#include <QString>
#include <QFile>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QVariant>

#include "fdt_model.h"

fdt_model::fdt_model(QWidget *parent) : QAbstractItemModel(parent) {
    QString dtb_file_name;
    dtb_file_name = QFileDialog::getOpenFileName(parent, tr("DTB file to open"), getenv("HOME"),
                                                 tr("Device tree blob files (*.dtb)"));

    fdt_model_from_file(dtb_file_name, parent);
}

fdt_model::fdt_model(const QString &dtb_file_name, QWidget *parent) : QAbstractItemModel(parent) {
    dtb_file = new QFile();
    fdt_model_from_file(dtb_file_name, parent);
}

fdt_model::fdt_model(QFile &dtb_f, QWidget *parent) : QAbstractItemModel(parent) {

    dtb_file = &dtb_f;
    if (!dtb_file->open(QIODevice::ReadOnly)) {
        QMessageBox::warning(parent, tr("Device Tree Display"),
                             tr("Unable to open file ") + dtb_file->fileName(),
                             QMessageBox::Cancel);
    } else {
        const struct fdt_header *dtb_ptr = reinterpret_cast<struct fdt_header *>(dtb_file->map(0, dtb_file->size()));
        uint32_t dtb_magic = dtb_ptr->magic;
        qDebug() << "dtb_magic = " << hex << dtb_magic;
    }
}

QModelIndex fdt_model::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    } else {
        TreeItem *parentItem;
        if (!parent.isValid()) {
            parentItem = root_item;
        } else {
            parentItem = static_cast<TreeItem*>(parent.internalPointer());
        }

        TreeItem *childItem = parentItem->child(row);
        if (childItem) {
            return createIndex(row, column, childItem);
        } else {
            return QModelIndex();
        }
    }
}

QModelIndex fdt_model::parent(const QModelIndex &index) const {
    if (!index.isValid()) {
        return QModelIndex();
    } else {
        TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
        TreeItem *parentItem = childItem->parentItem();

        if (parentItem == root_item) {
            return QModelIndex();
        } else {
            return createIndex(parentItem->row(), 0, parentItem);
        }
    }
}

int fdt_model::rowCount(const QModelIndex &parent) const {
    TreeItem *parentItem;
    if (parent.column() > 0) {
        return 0;
    } else {

        if (!parent.isValid()) {
            parentItem = root_item;
        } else {
            parentItem = static_cast<TreeItem*>(parent.internalPointer());
        }
        return parentItem->childCount();
    }
}

fdt_model::~fdt_model() {
    delete root_item;
}

int fdt_model::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    } else {
        return root_item->columnCount();
    }
}

QVariant fdt_model::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags fdt_model::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        return 0;
    } else {
        return QAbstractItemModel::flags(index);
    }
}

QVariant fdt_model::headerData(int section, Qt::Orientation orientation,
                               int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return root_item->data(section);
    } else {
        return QVariant();
    }
}

// Return indicator of whether this field is printable
static bool printable_values(char *val, size_t val_size) {
    QByteArray bytes(val, val_size);
    QVariant value(bytes);
    QStringList printable_strings;
    bool is_printable = true;

    // See if there are any non-printable characters is the area
    // This is checking for isprint or 0 bytes.
    for (auto i=bytes.begin(); i < bytes.end(); ++i) {
        char ch = *i;
        if (ch == 0) { // Special case, start with 0x00, assume non-printable
            if (i == bytes.begin()) {
                is_printable = false;
                break;
            } // endif
        } else if (!isprint(ch)) {
            is_printable = false;
            break;
        } // endif
    } // endfor
    return is_printable;
}

// Return a string list of either printable strings or printable hex representations
static QStringList print_values(char *val, size_t val_size) {
    QByteArray bytes(val, val_size);
    QVariant value(bytes);
    QStringList printable_strings;
    bool is_printable = printable_values(val, val_size);

    if (is_printable) {
        QString working_string;
        // Take the strings as entities split at 0x00 bytes.
        for (auto i=bytes.begin(); i < bytes.end(); ++i) {
            char ch = *i;
            if (ch == 0) { // end of this string.
                printable_strings.append('"' + working_string + '"');
                working_string.clear();
            } else {
                working_string.append(ch);
            } // endif
        } // endfor
    } else {
        // See if we format it as 32 bit unsigned ints or bytes
        if ((val_size & (sizeof(uint32_t)-1)) == 0) { // Can be done as "cells"
            QStringList cell_strings;
            cell_strings.append("<");
            for (unsigned int cell=0; cell < (val_size/sizeof(uint32_t)); ++cell) {
                QString cell_string;
                cell_string = "0x" + bytes.mid(cell*sizeof(uint32_t), sizeof(uint32_t)).toHex();
                if (cell < ((val_size/sizeof(uint32_t)-1))) {
                    cell_string += " ";
                }
                cell_strings.append(cell_string);
            }
            cell_strings.append(">");
            printable_strings.append(cell_strings.join(""));
        } else {
            // Format at array of bytes
            QStringList byte_strings;
            byte_strings.append("[");
            for (unsigned int o_byte=0; o_byte < val_size; ++o_byte) {
                QString byte_string;
                byte_string = "0x" + bytes.mid(o_byte, 1).toHex();
                byte_strings.append(byte_string);
            }
            byte_strings.append("]");
            printable_strings.append(byte_strings.join(""));
        } // endif
    } // endif
    return printable_strings;
}


static QString do_indent(uint indent_level, const QString indent_string) {
    QString ret_val;
    for (uint i=0; i < indent_level; ++i) {
        ret_val += indent_string;
    } // endfor
    return ret_val;
}

const fdt_node_header *fdt_model::fdt_model_process_node(const fdt_node_header *entry_fdt_n,
                                                         TreeItem *tree_node, QWidget *parent) {
    QString indent_str = "\t";
    QString printable_name;

    const fdt_node_header *fdt_n = entry_fdt_n;

    while (fdt_n->tag != FDT_END) {
        switch (fdt_n->tag) {
        case FDT_BEGIN_NODE: {
                uint node_header_len;
                node_header_len = strnlen((char *)fdt_n->name, fdt_hdr->size_dt_struct)+1; // for null;
                if (node_header_len <= 1) {
                    printable_name = "/";
                } else {
                    printable_name = QString::fromStdString(fdt_n->name);
                } // endif
                if ((tree_node->parentItem() == NULL) || (fdt_n != entry_fdt_n)) {
                    qDebug().noquote() << do_indent(indent_level, indent_str) << "fdt_n.tag = " << hex << fdt_n->tag <<
                                          " ftd_n name = " << printable_name;
                    ++indent_level;
                    QList <QVariant> node_data;
                    node_data << printable_name << "";
                    TreeItem *lower_node = new TreeItem(node_data, tree_node);
                    tree_node->appendChild(lower_node);
                    fdt_n = fdt_model_process_node(fdt_n, lower_node, parent);
                } else {
                    fdt_n = (fdt_node_header *)((uint8_t *)fdt_n +
                                               align(sizeof(*fdt_n)+node_header_len, sizeof(uint)));
                }
                }
            break;
        case FDT_END_NODE: {
                fdt_end_node_header *fdt_e = (fdt_end_node_header *)fdt_n;
                qDebug().noquote() << do_indent(indent_level, indent_str) << "fdt_n.tag = " << fdt_n->tag <<
                                      " end of current node";
                fdt_n = (fdt_node_header *)((uint8_t *)fdt_e + align(sizeof(*fdt_e), sizeof(uint)));
                --indent_level;
                return fdt_n;
            }
            break;
        case FDT_END:
            qDebug() << "End of structure tag = " << hex << fdt_n->tag;
            break;
        case FDT_NOP:
            break;
        case FDT_PROP: {
                struct fdt_property *fdt_p = new((uint8_t *)fdt_n) fdt_property;
                QStringList property_values;
                printable_name = QString::fromStdString(&string_table[fdt_p->nameoff]);
                property_values = print_values((char *) fdt_p + sizeof(*fdt_p), fdt_p->len);
                unsigned int prop_len = align(sizeof(*fdt_p) + fdt_p->len, sizeof(uint));

                QList <QVariant> node_data;
                node_data << printable_name;
                node_data << property_values.join(", ");
                TreeItem *lower_node = new TreeItem(node_data, tree_node);

                tree_node->appendChild(lower_node);

                fdt_n = (fdt_node_header *)((uint8_t *)fdt_n + prop_len);
                qDebug().noquote() << do_indent(indent_level, indent_str) << "fdt_p.tag = " << hex << fdt_p->tag <<
                                      " ftd_p name = " << printable_name << " = " << property_values;
            }
            break;
        default:
            break;
        } // endswitch
    } // endwhile
    return fdt_n;
}

void fdt_model::fdt_model_from_file(const QString &dtb_file_name, QWidget *parent) {

    dtb_file = new QFile();

    if (!dtb_file_name.isEmpty()) {
        dtb_file->setFileName(dtb_file_name);
        if (!dtb_file->open(QIODevice::ReadOnly)) {
            QMessageBox::warning(parent, tr("Device Tree Display"),
                                 tr("Unable to open file ") + QString(dtb_file_name),
                                 QMessageBox::Cancel);
        } else {
            void * dtb_ptr = dtb_file->map(0, dtb_file->size());
            fdt_hdr = new(dtb_ptr) fdt_header;
            uint32_t magic = fdt_hdr->magic;
            uint32_t version = fdt_hdr->version;
            uint32_t struct_offset = fdt_hdr->off_dt_struct;
            uint32_t strings_offset = fdt_hdr->off_dt_strings;
            QString printable_name;

            qDebug() << "magic = " << hex << magic;
            qDebug() << "version = " << version;
            qDebug() << "struct_offset = 0x" << hex << struct_offset;
            qDebug() << "string_offset = 0x" << hex << strings_offset;
            string_table = new((uint8_t *)dtb_ptr + strings_offset) char[fdt_hdr->totalsize-fdt_hdr->off_dt_strings];

            fdt_rsvmap = new((uint8_t *)dtb_ptr+fdt_hdr->off_mem_rsvmap) fdt_reserve_entry;
            for (auto i=0; ; ++i) {
                qDebug() << "Reserved memory map " << hex << fdt_rsvmap[i].address << " Size " << fdt_rsvmap[i].size;
                if ((fdt_rsvmap[i].address == 0) && (fdt_rsvmap[i].size == 0)) {
                    break;
                } // endif
            } // endfor

            const fdt_node_header *fdt_root_n = new((uint8_t *)dtb_ptr+fdt_hdr->off_dt_struct) fdt_node_header;

            indent_level = 0;
            qDebug() << "fdt_root_n = " << hex << fdt_root_n;
            if (fdt_root_n->name[0] == 0) {
                printable_name = "/";
            } else {
                printable_name = fdt_root_n->name;
            }

            QList<QVariant> root_data;
            QString home_dir_path = QDir::homePath();
            QString title_file_name = dtb_file->fileName();

            if (title_file_name.remove(home_dir_path,Qt::CaseSensitive).length() != 0) {
                title_file_name = "${HOME}" + title_file_name;
            }
            root_data << title_file_name << "Flattened Device Tree";
            root_item = new TreeItem(root_data);

            fdt_model_process_node(fdt_root_n, root_item, parent);

        } // endif
    } // endif
}
