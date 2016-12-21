make 
insmod f2fs.ko
mkfs.f2fs -l f2fs /dev/sdb1  
mount -t f2fs /dev/sdb1 /mnt/newdsik
./f2fsioc /mnt/newdsik
cp ../1.txt /mnt/newdsik/1
