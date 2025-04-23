import lfs_test_lib

cfg = lfs_test_lib.LFSConfig()
cfg.read_size = 16
cfg.prog_size = 16
cfg.block_size = 320
cfg.block_count = 3
cfg.cache_size = 2 * cfg.prog_size
cfg.lookahead_size = cfg.prog_size
cfg.block_cycles = 500
cfg.inline_max = cfg.cache_size

test_lib = lfs_test_lib.LfsTestLib("flash.img", cfg)
test_lib.mount()
file1 = lfs_test_lib.lfs_file_t()

test_lib.file_open(file1, "file1.txt", lfs_test_lib.O_RDWR | lfs_test_lib.O_CREAT)

msg = "1111111111111111111111111"
test_lib.file_write(file1, msg, len(msg))

buf = bytearray(100)
test_lib.file_rewind(file1)  # Rewind the file pointer to the beginning
print(test_lib.file_tell(file1))  # Check the file pointer position
file_size = test_lib.file_read(file1, buf)
print("Read data: ", buf[:file_size])
print("File pointer after read: ", test_lib.file_tell(file1))

test_lib.file_close(file1)
test_lib.unmount()
