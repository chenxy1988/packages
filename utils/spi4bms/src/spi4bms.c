/*
 * SPI Seal layer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev1.1";
static uint32_t mode;
static uint8_t bits = 8;
static uint32_t speed = 500000;
static uint16_t delay;
static int verbose;
static int transfer_size;
static int iterations;

static int spifd = -1;
static void default_setup();

static void spi_terminate()
{
	close(spifd);
	spifd = -1;
}

static void spi_init()
{
        int ret = 0;
        int fd;

        fd = open(device, O_RDWR);
        if (fd < 0)
                pabort("can't open device");

	default_setup();
	/*
         * spi mode
         */
        ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
        if (ret == -1)
                pabort("can't set spi mode");

        ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
        if (ret == -1)
                pabort("can't get spi mode");
	 /*
         * bits per word
         */
        ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
        if (ret == -1)
                pabort("can't set bits per word");

        ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
        if (ret == -1)
                pabort("can't get bits per word");

	/*
         * max speed hz
         */
        ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
        if (ret == -1)
                pabort("can't set max speed hz");

        ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
        if (ret == -1)
                pabort("can't get max speed hz");

        printf("spi mode: 0x%x\n", mode);
        printf("bits per word: %d\n", bits);
        printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	spifd = fd;
}

static void default_setup()
{
//[1] Mode config
	mode |= SPI_CPHA;
	mode |= SPI_CPOL; //mode = 4, SPI_CPHA=1 SPI_CPOL=1
//[2] Debug info
	verbose = 1;
//[3] Delay
	delay = 10;//10us for line response
}

static void speed_setup(int speed)
{
	int ret;
        ret = ioctl(spifd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
        if (ret == -1)
                pabort("can't set max speed hz");

        ret = ioctl(spifd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
        if (ret == -1)
                pabort("can't get max speed hz");	
}

static void cs_setup(int cs)
{
	int ret ;

	if(cs == 1){
		mode |= SPI_CS_HIGH;
	}else{
		mode &= ~SPI_CS_HIGH;
	}
	/*
         * spi mode
         */
        ret = ioctl(spifd, SPI_IOC_WR_MODE32, &mode);
        if (ret == -1)
                pabort("can't set spi mode");

        ret = ioctl(spifd, SPI_IOC_RD_MODE32, &mode);
        if (ret == -1)
                pabort("can't get spi mode");
}

static void hex_dump(const void *src, size_t length, size_t line_size,
		     char *prefix)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;

	printf("%s | ", prefix);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" | ");  /* right close */
			while (line < address) {
				c = *line++;
				printf("%c", (c < 33 || c == 255) ? 0x2E : c);
			}
			printf("\n");
			if (length > 0)
				printf("%s | ", prefix);
		}
	}
}

static void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	int ret;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

	if (verbose){
		hex_dump(tx, len, 32, "TX");
		hex_dump(rx, len, 32, "RX");
	}
}



int main(int argc, char *argv[])
{
	unsigned char t = 0xfa;
	unsigned char r;

	spi_init();
	transfer(spifd,&t,&r,1);
	spi_terminate();
	return 0;
}
