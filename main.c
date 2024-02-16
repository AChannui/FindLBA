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

} partition_t ;

chs_t parseChs(const uint8_t* source){
    chs_t result;
    result.head = source[0];
    result.sector = source[1] & 0b00111111;
    result.cylinder = source[1] & 0b11000000;
    result.cylinder = result.cylinder << 2;
    uint16_t temp = source[2];
    result.cylinder += temp;
    return result;
}

uint32_t parseNum_sectors(const uint8_t* source){
    uint32_t result;
    result = source[3];
    result = result << 8;
    result = source[2] | result;
    result = result << 8;
    result = source[1] | result;
    result = result << 8;
    result = source[0] | result;
    return result;
}

partition_t parsePartition(uint8_t* source){
    partition_t result;
    result.boot_flag = source[0];
    result.start_chs = parseChs(&source[1]);
    result.type_code = source[4];
    result.end_chs = parseChs(&source[5]);
    result.lba = parseNum_sectors(&source[8]);
    //ntohl doesn't work wrong order
    //result.lba = ntohl(*((uint32_t*) &source[8]));
    // this works because of machine endianness
    //result.lba = (*((uint32_t*) &source[8]));
    result.num_sectors = parseNum_sectors(&source[12]);
    return result;
}

uint32_t getPartAddr(int fd){
    const int BLOCK_SIZE = 512;
    uint8_t mbr[BLOCK_SIZE];
    ssize_t read_check = read(fd, mbr, BLOCK_SIZE);
    if(read_check != BLOCK_SIZE){
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

void readSuperBlock(int fd, int off){
    lseek(fd, off, SEEK_SET);

}

void help(const char *name){
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
    if(drive_fd < 0){
        printf("Failed to open drive '%s': %s (errno=%d)\n", drive_name, strerror(errno), errno);
    }

    // get partition address
    uint32_t part_address = getPartAddr(drive_fd);



    return EXIT_SUCCESS;
}

