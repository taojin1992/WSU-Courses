#for checking results of mkdir and creat, diff -s file1 file2
sudo mount -o loop mydisk /mnt
ls -l /mnt
sudo umount /mnt
