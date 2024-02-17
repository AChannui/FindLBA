#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    uint8_t head;
    uint8_t sector;
    uint16_t cylinder;
} chs_t;

typedef struct {
    uint8_t boot_flag;
    chs_t start_chs;
    uint8_t type_code;
    chs_t end_chs;
    uint32_t lba;
    uint32_t num_sectors;

} partition_t;

typedef struct {
    uint32_t inodes;
    uint32_t blocks;
    uint16_t magic_sig;
    uint32_t block_size;

} super_block_t;

chs_t parseChs(const uint8_t *source) {
    chs_t result;
    result.head = source[0];
    result.sector = source[1] & 0b00111111;
    result.cylinder = source[1] & 0b11000000;
    result.cylinder = result.cylinder << 2;
    uint16_t temp = source[2];
    result.cylinder += temp;
    return result;
}

partition_t parsePartition(const uint8_t *source) {
    partition_t result;
    result.boot_flag = source[0];
    result.start_chs = parseChs(&source[1]);
    result.type_code = source[4];
    result.end_chs = parseChs(&source[5]);
    result.lba = *(uint32_t *) (&source[8]);
    //result.lba = parseNum_sectors(&source[8]);
    //ntohl doesn't work wrong order
    //result.lba = ntohl(*((uint32_t*) &source[8]));
    // this works because of machine endianness
    //result.lba = (*((uint32_t*) &source[8]));
    result.num_sectors = *(uint32_t *) (&source[12]);
    //result.num_sectors = parseNum_sectors(&source[12]);
    return result;
}

super_block_t parseSuperBlock(const uint8_t *source) {
    super_block_t result;
    result.inodes = *(uint32_t *) (&source[0]);
    result.blocks = *(uint32_t *) (&source[4]);
    uint32_t temp = *(uint32_t *) (&source[0x18]);
    result.block_size = 1 << (temp + 10);
    result.magic_sig = *(uint16_t *) (&source[0x38]);
    return result;
}


uint32_t getPartAddr(int fd) {
    const int BLOCK_SIZE = 512;
    uint8_t mbr[BLOCK_SIZE];
    ssize_t read_check = read(fd, mbr, BLOCK_SIZE);
    if (read_check != BLOCK_SIZE) {
        printf("Read not enough bytes from drive %zd\n", read_check);
        exit(EXIT_FAILURE);
    }
    const int PARTITION_TABLE_START = 440 //code
                                      + 4 //disk signature
                                      + 2 //nulls
    ;
    partition_t part = parsePartition(&mbr[PARTITION_TABLE_START]);
    uint32_t address = part.lba * BLOCK_SIZE;
    printf("LBA partition 1 : %#010x\n", address);
    return address;
}

void readSuperBlock(int fd, off_t off) {
    const int BLOCK_SIZE = 4096;
    uint8_t buff[BLOCK_SIZE];
    lseek(fd, off, SEEK_SET);
    ssize_t read_check = read(fd, buff, BLOCK_SIZE);
    if (read_check != BLOCK_SIZE) {
        printf("Read not enough bytes from super block %zd\n", read_check);
        exit(EXIT_FAILURE);
    }
    super_block_t super_block = parseSuperBlock(buff);
    printf("Magic signature : %x\n", super_block.magic_sig);
    printf("Size of block : %d\n", super_block.block_size);
    printf("Total number of blocks : %d\n", super_block.blocks);
    printf("Total number of inodes : %d\n", super_block.inodes);
}

void help(const char *name) {
    fprintf(stderr, "Usage: %s [-d device] [-h]\n", name);
}


int main(int argc, char *argv[]) {
    // get file name
    int opt;
    char *drive_name = "/dev/sdb";
    while ((opt = getopt(argc, argv, "d:h")) != -1) {
        switch (opt) {
            case 'd':
                drive_name = optarg;
                break;
            case 'h':
                help(argv[0]);
                exit(EXIT_SUCCESS);
            default: /* '?' */
                help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        fprintf(stderr, "To many arguments after options\n");
        exit(EXIT_FAILURE);
    }
    // open file
    printf("opening device %s\n", drive_name);
    int drive_fd = open(drive_name, O_RDONLY);
    if (drive_fd < 0) {
        printf("Failed to open drive '%s': %s (errno=%d)\n", drive_name, strerror(errno), errno);
    }

    // get partition address
    uint32_t part_address = getPartAddr(drive_fd);
    readSuperBlock(drive_fd, part_address + 1024);
    return EXIT_SUCCESS;
}

