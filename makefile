all:
	echo "Hello to, littlefs demo"
create:
	wget https://github.com/littlefs-project/littlefs/archive/refs/tags/v2.5.0.zip
	unzip v2.5.0.zip
	rm v2.5.0.zip
clean:
	rm *.o
	rm -f v2.5.0.zip
	rm -rf littlefs-2.5.0
build:
	gcc -c main.c -o main.o -I./littlefs-2.5.0
	gcc -c littlefs-2.5.0/lfs.c -o lfs.o -D LFS_NO_MALLOC
	gcc -c littlefs-2.5.0/lfs_util.c -o lfs_util.o -D LFS_NO_MALLOC
	gcc -o app main.o lfs.o lfs_util.o
	rm *.o
run: build
	./app