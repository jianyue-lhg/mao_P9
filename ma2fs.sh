cd  /root/share/mao_P9
make
fuser -km /mnt/newdsik
umount /mnt/newdsik
rmmod f2fs 
insmod f2fs.ko
mkfs.f2fs -l f2fs /dev/sdb1
mount -t f2fs /dev/sdb1 /mnt/newdsik

./f2fsioc /mnt/newdsik
cp /root/share/1.txt /mnt/newdsik/1
cp /root/share/1.txt /mnt/newdsik/2
cp /root/share/1.txt /mnt/newdsik/3
cp /root/share/1.txt /mnt/newdsik/4
cp /root/share/1.txt /mnt/newdsik/5
#cp /root/share/1.txt /mnt/newdsik/6
#cp /root/share/1.txt /mnt/newdsik/7
#cp /root/share/1.txt /mnt/newdsik/8
#cp /root/share/1.txt /mnt/newdsik/9
#cp /root/share/1.txt /mnt/newdsik/10


