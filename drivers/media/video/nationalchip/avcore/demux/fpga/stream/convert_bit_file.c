#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

enum {
	FILE_DATA,
	FILE_SYNC,
};

enum {
	CVT_1BIT,
	CVT_2BIT,
	CVT_4BIT,
};

static int fd;
static int byte_count = 0;
extern unsigned char g_dvr_fpga_stream[];
extern unsigned int  g_dvr_fpga_stream_len;
extern unsigned char parallel_sync_txt[];
extern unsigned int  parallel_sync_txt_len;

int bit_file_write_byte(int type, char byte)
{
	char convert_byte;
	char convert_mask;
	unsigned char buffer[10];
	int i, convert_shift, loop = 8/(1<<type);
	if (type == CVT_1BIT) {
		convert_mask  = 0x1;
		convert_shift = 0x1;

	} else if (type == CVT_2BIT) {
		convert_mask  = 0x3;
		convert_shift = 0x2;

	} else {
		convert_mask  = 0xf;
		convert_shift = 0x4;
	}

	for (i = 0; i < loop; i++) {
		if (byte_count == 0)
			write(fd, "\t", 1);

		convert_byte = (byte>>(convert_shift*(loop-i-1))) & convert_mask;
		sprintf(buffer, "0x0%x,", convert_byte);
		write(fd, buffer, 5);

		byte_count++;
		if (byte_count%12 == 0) {
			write(fd, "\n", 1);
			byte_count = 0;

		} else {
			write(fd, " ", 1);
		}
	}
}

int bit_file_write_sync(int type, char byte)
{
	int i, loop = 8/(1<<type);

	for (i = 0; i < loop; i++) {
		if (byte_count == 0)
			write(fd, "\t", 1);

		if (byte)
			write(fd, "0x01,", 5);
		else
			write(fd, "0x00,", 5);

		byte_count++;
		if (byte_count%12 == 0) {
			write(fd, "\n", 1);
			byte_count = 0;

		} else {
			write(fd, " ", 1);
		}
	}
}

int bit_file_init(int file_type, int type)
{
	int align;
	unsigned char file[5];
	unsigned char buffer[80];
	sprintf(file, "%s", file_type ? "sync" : "data");
	sprintf(buffer, "%dbit_%s.c", 1<<type, file);
	fd = open(buffer, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
	printf("create %s, ret %d\n", buffer, fd);

	align = 32/(1<<type);
	sprintf(buffer, "unsigned char __attribute__ ((aligned (%2d))) serial_%s_%dbit_txt[] = {\n", align, file, 1<<type);
	write(fd, buffer, 72);
	byte_count = 0;
}

int bit_file_deinit(int file_type, int type)
{
	unsigned char file[5];
	unsigned char buffer[80];
	sprintf(file, "%s", file_type ? "sync" : "data");
	write(fd, "};\n", 3);

	sprintf(buffer, "unsigned int serial_%s_%dbit_txt_len = sizeof(serial_%s_%dbit_txt);\n",
			file, 1<<type, file, 1<<type);

	write(fd, buffer, 70);
	close(fd);
}

int bit_file_create(int file_type)
{
	int i, j;
	unsigned char *stream = file_type ? parallel_sync_txt : g_dvr_fpga_stream;
	unsigned int stream_len = file_type ? parallel_sync_txt_len : g_dvr_fpga_stream_len;

	for (i = 0; i < CVT_4BIT+1; i++) {
		bit_file_init(file_type, i);
		for (j = 0; j < stream_len; j++) {
			if (file_type)
				bit_file_write_sync(i, stream[j]);
			else
				bit_file_write_byte(i, stream[j]);
		}
		write(fd, "\n", 1);
		bit_file_deinit(file_type, i);
	}
}

int main(int argc, char **argv)
{
	bit_file_create(FILE_DATA);
	bit_file_create(FILE_SYNC);
}
