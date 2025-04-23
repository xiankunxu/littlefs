/* lfs_filebd.c  – littlefs block‑device backed by a host file
 *
 *  Build:  gcc -Wall -Wextra -std=c11 -c lfs_filebd.c
 *  Link with your test app together with littlefs sources.
 *
 *  Author: 2025‑04  (public domain / MIT, do what you like)
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <pybind11/pybind11.h>
#include "lfs.h"

/* --------------------------------------------------------------------------
 * User‑visible API
 * --------------------------------------------------------------------------*/
typedef struct {
    const char *path;      /* backing file path                     */
    int         fd;        /* open() file descriptor                */
} lfs_filebd_config_t;

/* --------------------------------------------------------------------------
 * Implementation
 * --------------------------------------------------------------------------*/

/* ---- littlefs callbacks ----------------------------------------------- */
static int lfs_filebd_read(const struct lfs_config *c,
                           lfs_block_t block, lfs_off_t off,
                           void *buffer, lfs_size_t size)
{
    const lfs_filebd_config_t *bd = static_cast<const lfs_filebd_config_t *>(c->context);
    off_t pos = (off_t)block * c->block_size + off;

    if (pread(bd->fd, buffer, size, pos) != (ssize_t)size)
        return LFS_ERR_IO;
    return LFS_ERR_OK;
}

static int lfs_filebd_prog(const struct lfs_config *c,
                           lfs_block_t block, lfs_off_t off,
                           const void *buffer, lfs_size_t size)
{
    const lfs_filebd_config_t *bd = static_cast<const lfs_filebd_config_t *>(c->context);
    off_t pos = (off_t)block * c->block_size + off;

    /* littlefs guarantees we only program erased bytes (0xFF→0) */
    if (pwrite(bd->fd, buffer, size, pos) != (ssize_t)size)
        return LFS_ERR_IO;
    return LFS_ERR_OK;
}

static int lfs_filebd_erase(const struct lfs_config *c,
                            lfs_block_t block)
{
    const lfs_filebd_config_t *bd = static_cast<const lfs_filebd_config_t *>(c->context);
    off_t pos = (off_t)block * c->block_size;

    /* Erase = write 0xFF over the block */
    static uint8_t ff[512];
    memset(ff, 0xFF, sizeof(ff));

    size_t remain = c->block_size;
    while (remain) {
        size_t chunk = remain < sizeof(ff) ? remain : sizeof(ff);
        if (pwrite(bd->fd, ff, chunk, pos) != (ssize_t)chunk)
            return LFS_ERR_IO;
        pos    += chunk;
        remain -= chunk;
    }
    return LFS_ERR_OK;
}

static int lfs_filebd_sync(const struct lfs_config *c)
{
    const lfs_filebd_config_t *bd = static_cast<const lfs_filebd_config_t *>(c->context);
    if (fsync(bd->fd) < 0)
        return LFS_ERR_IO;
    return LFS_ERR_OK;
}


//int main(void)
//{
//    lfs_t lfs;
//    struct lfs_config cfg;
//    lfs_filebd_config_t bd;
//
//    /* Create 128 kB “flash” in file flash.img, 4 kB blocks */
//    if (lfs_filebd_create("flash.img", 320, 2, &cfg, &bd) != LFS_ERR_OK) {
//        return 1;
//    }
//
//    /* Mount (or format + mount) */
//    int err = lfs_mount(&lfs, &cfg);
//    if (err) {
//        lfs_format(&lfs, &cfg);
//        err = lfs_mount(&lfs, &cfg);
//        if (err) {
//            puts("mount failed");
//            return 1;
//        }
//    }
//
//    /* Simple test */
//    lfs_file_t file1;
//    lfs_file_open(&lfs, &file1, "file1.txt", LFS_O_RDWR | LFS_O_CREAT );
//    const char msg[] = "111111111111111";
//    lfs_file_write(&lfs, &file1, msg, sizeof(msg)-1);
//    const char msg2[] = "222222222222222";
//    lfs_file_write(&lfs, &file1, msg2, sizeof(msg2)-1);
//
//    char buf[48];
//    lfs_file_read(&lfs, &file1, buf, sizeof(buf)-1);
//    buf[sizeof(buf)-1] = '\0';
//    printf("Read from file1: %s\n", buf);
//
//
////    lfs_file_t file2;
////    lfs_file_open(&lfs, &file2, "file2.txt", LFS_O_RDWR | LFS_O_CREAT );
////    const char msg[] = "22222222222222222222aaaaaaaaaaaaaaaaaaaaaaaabb\0";
////    lfs_file_write(&lfs, &file2, msg, sizeof(msg)-1);
////
////    lfs_file_read(&lfs, &file2, buf, sizeof(buf)-1); // Uncommented to read from file2
////    printf("Read from file2: %s\n", buf); // Uncommented to print read data
//
//
//    lfs_file_close(&lfs, &file1);
////    lfs_file_close(&lfs, &file2);
//
//    lfs_unmount(&lfs);
//    lfs_filebd_destroy(&bd);
//    return 0;
//}

class LfsTesetLib {
private:
    lfs_t lfs_;
    struct lfs_config cfg_;
    lfs_filebd_config_t bd_;
public:
    LfsTesetLib(const char* path,
                struct lfs_config cfg) {

        memset(&bd_, 0, sizeof(bd_));
        bd_.path       = path;
        /* Open file (create if missing) */
        bd_.fd = open(bd_.path, O_RDWR | O_CREAT, 0644);
        if (bd_.fd < 0) {
            std::cout << "open backing file error" << std::endl;
            return;
        }

        /* Fill lfs_config */
        memset(&cfg_, 0, sizeof(cfg_));
        cfg_.context      = &bd_;
        cfg_.read         = lfs_filebd_read;
        cfg_.prog         = lfs_filebd_prog;
        cfg_.erase        = lfs_filebd_erase;
        cfg_.sync         = lfs_filebd_sync;
        cfg_.read_size    = cfg.read_size;
        cfg_.prog_size    = cfg.prog_size;
        cfg_.block_size   = cfg.block_size;
        cfg_.block_count  = cfg.block_count;
        cfg_.cache_size   = 2 * cfg_.prog_size;
        cfg_.lookahead_size = cfg.lookahead_size;
        cfg_.block_cycles = cfg.block_cycles;

        /* Ensure the file is exactly block_size*block_count bytes */
        off_t need = (off_t)cfg_.block_size * cfg_.block_count;
        if (ftruncate(bd_.fd, need) < 0) {
            std::cout << "ftruncate error" << std::endl;
            close(bd_.fd);
            return;
        }
    }

    ~LfsTesetLib() {
        if (bd_.fd >= 0) close(bd_.fd);
    }

    int mount() {
        int err = lfs_mount(&lfs_, &cfg_);
        if (err) {
            lfs_format(&lfs_, &cfg_);
            err = lfs_mount(&lfs_, &cfg_);
            if (err) {
                std::cout << "mount failed" << std::endl;
                return 1;
            }
        }
        return 0;
    }

    int file_open(lfs_file_t *file, const char* name, int flags) {
        return lfs_file_open(&lfs_, file, name, flags);
    }

    int file_write(lfs_file_t *file, const char* buffer, lfs_size_t size) {
        return lfs_file_write(&lfs_, file, buffer, size);
    }

    int file_read(lfs_file_t *file, char* buffer, lfs_size_t size) {
        return lfs_file_read(&lfs_, file, buffer, size);
    }

    int file_close(lfs_file_t *file) {
        return lfs_file_close(&lfs_, file);
    }

    int unmount() {
        return lfs_unmount(&lfs_);
    }

    int file_rewint(lfs_file_t* file) {
        return lfs_file_rewind(&lfs_, file);
    }

    /* whence should be a value of lfs_whence_flags */
    int file_seek(lfs_file_t* file, lfs_off_t offset, int whence) {
        return lfs_file_seek(&lfs_, file, offset, whence);
    }

    int file_tell(lfs_file_t* file) {
        return lfs_file_tell(&lfs_, file);
    }

    int file_sync(lfs_file_t* file) {
        return lfs_file_sync(&lfs_, file);
    }
};


namespace py = pybind11;
PYBIND11_MODULE(lfs_test_lib, m) {
    py::class_<struct lfs_config>(m, "LFSConfig")
        .def(py::init<>())
        .def_readwrite("read_size", &lfs_config::read_size)
        .def_readwrite("prog_size", &lfs_config::prog_size)
        .def_readwrite("block_size", &lfs_config::block_size)
        .def_readwrite("block_count", &lfs_config::block_count)
        .def_readwrite("cache_size", &lfs_config::cache_size)
        .def_readwrite("lookahead_size", &lfs_config::lookahead_size)
        .def_readwrite("block_cycles", &lfs_config::block_cycles)
        .def_readwrite("inline_max", &lfs_config::inline_max);
    
    py::class_<lfs_file_t>(m, "lfs_file_t")
        .def(py::init<>());

    /* py::arithmetic() which tells it to inject all the usual integer operators (including |, &, ^, etc.) for the enum */
    py::enum_<lfs_open_flags>(m, "LFSOpenFlags", py::arithmetic())
        .value("O_RDONLY",  LFS_O_RDONLY)
        .value("O_WRONLY",  LFS_O_WRONLY)
        .value("O_RDWR",    LFS_O_RDWR)
        .value("O_CREAT",   LFS_O_CREAT)
        .value("O_EXCL",    LFS_O_EXCL)
        .value("O_TRUNC",   LFS_O_TRUNC)
        .value("O_APPEND",  LFS_O_APPEND)
        .export_values();   // optional: let Python lookups like ImageState.FACTORY
    
    py::enum_<lfs_whence_flags>(m, "LFSWhenceFlags")
        .value("SEEK_SET",  LFS_SEEK_SET)
        .value("SEEK_CUR",  LFS_SEEK_CUR)
        .value("SEEK_END",  LFS_SEEK_END)
        .export_values();   // optional: let Python lookups like ImageState.FACTORY
    
    py::class_<LfsTesetLib>(m, "LfsTestLib")
        .def(py::init<const char*, struct lfs_config>())
        .def("mount", &LfsTesetLib::mount, "Mount littlefs")
        /* Binding a function that takes a raw pointer is almost identical to binding one that takes a reference pybind11 will unwrap the Python lfs_file_t object and pass its address. */
        .def("file_open", &LfsTesetLib::file_open, "Open a file in littlefs", py::arg("file").none(false), py::arg("name"), py::arg("flags"))
        .def("file_write", &LfsTesetLib::file_write, "Write to a file in littlefs", py::arg("file").none(false), py::arg("buffer"), py::arg("size"))
        .def(   "file_read", 
                [](LfsTesetLib &self, lfs_file_t* file, py::buffer buf) {
                    // Request a buffer_info from the Python object
                    auto info = buf.request();
                    // We only support 1‑D byte buffers here
                    if (info.ndim != 1 || info.itemsize != 1)
                        throw std::runtime_error("read_data requires a 1‑D byte buffer");
                    size_t capacity = info.shape[0];

                    char* data = static_cast<char*>(info.ptr);
                    size_t written = self.file_read(file, data, capacity);

                    // Return number of bytes actually written
                    return written;
                },
                "Read from a file in littlefs",
                py::arg("file").none(false),
                py::arg("buf")
            )
        .def("file_close", &LfsTesetLib::file_close, "Close a file in littlefs", py::arg("file").none(false))
        .def("unmount", &LfsTesetLib::unmount, "Unmount littlefs")
        .def("file_tell", &LfsTesetLib::file_tell, "Get the current position in a file", py::arg("file").none(false))
        .def("file_seek", &LfsTesetLib::file_seek, "Set the position of the file pointer", py::arg("file").none(false), py::arg("offset"), py::arg("whence"))
        .def("file_rewind", &LfsTesetLib::file_rewint, "Rewind a file in littlefs", py::arg("file").none(false))
        .def("file_sync", &LfsTesetLib::file_sync, "Sync a file in littlefs", py::arg("file").none(false))
        ;
}
