cp /lib/modules/$(uname -r)/build/include/trace/events/f2fs.h /lib/modules/$(uname -r)/build/include/trace/events/f2fs.h.bak
cp f2fs_trace.h /lib/modules/$(uname -r)/build/include/trace/events/f2fs.h
cp /lib/modules/$(uname -r)/build/include/linux/f2fs_fs.h /lib/modules/$(uname -r)/build/include/linux/f2fs_fs.h.bak
cp f2fs_fs.h  /lib/modules/$(uname -r)/build/include/linux/f2fs_fs.h
