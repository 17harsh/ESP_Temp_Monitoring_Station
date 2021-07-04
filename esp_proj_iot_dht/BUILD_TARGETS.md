build_clean	-	make clean

build_debug	-	make COMPILE=gcc BOOT=none APP=0 SPI_SPEED=40 SPI_MODE=QIO SPI_SIZE_MAP=4 FLAVOR=debug

build_release	-	make COMPILE=gcc BOOT=none APP=0 SPI_SPEED=40 SPI_MODE=QIO SPI_SIZE_MAP=4 FLAVOR=release

mem_erase	-	make erase

mem_flash	-	make flash
