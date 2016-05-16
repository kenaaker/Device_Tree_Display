#ifndef FDT_CLASS_H
#define FDT_CLASS_H
#include <QWidget>
#include <QFile>
#include <QString>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include <stdint.h>

#include "treeitem.h"

#include "fixedendian.h"
typedef BigEndian<uint32_t> fdt32_t;
typedef BigEndian<uint64_t> fdt64_t;

struct fdt_header {
    fdt32_t magic;			 /* magic word FDT_MAGIC */
    fdt32_t totalsize;		 /* total size of DT block */
    fdt32_t off_dt_struct;		 /* offset to structure */
    fdt32_t off_dt_strings;		 /* offset to strings */
    fdt32_t off_mem_rsvmap;		 /* offset to memory reserve map */
    fdt32_t version;		 /* format version */
    fdt32_t last_comp_version;	 /* last compatible version */

    /* version 2 fields below */
    fdt32_t boot_cpuid_phys;	 /* Which physical CPU id we're
                        booting on */
    /* version 3 fields below */
    fdt32_t size_dt_strings;	 /* size of the strings block */

    /* version 17 fields below */
    fdt32_t size_dt_struct;		 /* size of the structure block */
};

struct fdt_reserve_entry {
    fdt64_t address;
    fdt64_t size;
};

struct fdt_node_header {
    fdt32_t tag;
    char name[0];
};

struct fdt_end_node_header {
    fdt32_t tag;
};

struct fdt_property {
    fdt32_t tag;
    fdt32_t len;
    fdt32_t nameoff;
};

#define FDT_MAGIC	0xd00dfeed	/* 4: version, 4: total size */
#define FDT_TAGSIZE	sizeof(fdt32_t)

#define FDT_BEGIN_NODE	0x1		/* Start node: full name */
#define FDT_END_NODE	0x2		/* End node */
#define FDT_PROP	0x3		/* Property: name off,
                       size, content */
#define FDT_NOP		0x4		/* nop */
#define FDT_END		0x9

#define FDT_V1_SIZE	(7*sizeof(fdt32_t))
#define FDT_V2_SIZE	(FDT_V1_SIZE + sizeof(fdt32_t))
#define FDT_V3_SIZE	(FDT_V2_SIZE + sizeof(fdt32_t))
#define FDT_V16_SIZE	FDT_V3_SIZE
#define FDT_V17_SIZE	(FDT_V16_SIZE + sizeof(fdt32_t))

class fdt_model : public QAbstractItemModel {
    Q_OBJECT
public:
    fdt_model(QWidget *parent = 0);
    fdt_model(const QString &dtb_file_name, QWidget *parent = 0);
    fdt_model(QFile &dtb_f, QWidget *parent);
    ~fdt_model();
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

private:
    struct fdt_header *fdt_hdr;
    struct fdt_reserve_entry *fdt_rsvmap;
    char *string_table;
    uint align(uint v, uchar bdy) {
        return ((v+(bdy-1) & (~(bdy-1))));
    }

signals:

public slots:

private:
    void fdt_model_from_file(const QString &dtb_file_name, QWidget *parent);
    const fdt_node_header *fdt_model_process_node(const fdt_node_header *, TreeItem *, QWidget *);
    QFile *dtb_file;
    TreeItem *root_item;
    uint indent_level;
};

#endif // FDT_CLASS_H
