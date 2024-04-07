#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
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

super_block_t readSuperBlock(int fd, off_t off) {
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
    return super_block;
}

int check_indirect(const unsigned char block[], const unsigned int block_size, unsigned int total_blocks, int current_block) {
    int consecutive = 0;
    uint32_t last_numb = 0;
    int index;

    const uint32_t *indirects = (const uint32_t *) (block);
    unsigned int indirects_size = block_size / sizeof(indirects[0]);

    for (index = 0; index < 32; index++) {
        const uint32_t current_numb = indirects[index];
        if (current_numb == 0) {
            break;
        }
        if (current_numb >= total_blocks) {
            return 0;
        }
        if (last_numb + 1 == current_numb) {
            consecutive++;
        } else {
            consecutive = 0;
        }
        if (consecutive > 4) {
            return 1;
        }

        last_numb = current_numb;
    }
    // assuming the first address in the indirect blocks address is right after current block address
    if (current_block != 0 && indirects[0] != current_block + 1) {
        return 0;
    }
    if (index == 32) {
        return 0;
    }
    // need at least one valid address in indirect
    if (index == 0) {
        return 0;
    }
    for (; index < indirects_size; index++) {
        // all addresses after first 0 must be 0s
        if (indirects[index] != 0) {
            return 0;
        }
    }
    return 2;
}

int check_start(const unsigned char block[], const ssize_t  buff_size){
    int header_size = 12;
    if(buff_size < header_size){
        return 0;
    }
    // start of riff
    if(memcmp(&block[0], "RIFF", 4) != 0){
        return 0;
    }

    if(memcmp(&block[8], "WEBP", 4) != 0){
        return 0;
    }
    return 1;
}

void check_part(const super_block_t super_block, int fd, off_t off, int print_indirects, int ensure_address,
                long block_break) {
    int start_count = 0;
    int indirect_count = 0;
    lseek(fd, off, SEEK_SET);
    for (int current_block = 0; current_block < super_block.blocks; current_block++) {
        unsigned char buff[super_block.block_size];
        if (block_break == current_block) {
            printf("reached block: %d\n", current_block);
        }
        const ssize_t bytes_read = read(fd, buff, super_block.block_size);
        const int tmp = check_indirect(buff, super_block.block_size, super_block.blocks,
                                       ensure_address ? current_block : 0);

        if (tmp) {
            if (print_indirects) {
                printf("indirect block: %d", current_block);
            }
            if (tmp == 2) {
                printf(", fake block\n");
            } else {
                printf("\n");
            }
            indirect_count++;
        }
        if(check_start(buff, bytes_read)){
            start_count++;
            printf("start block: %d\n", current_block);
        }
    }
    printf("Total start blocks: %d\n", start_count);
    printf("Total indirect blocks: %d\n", indirect_count);

}

void help(const char *name) {
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "   -h          print this help\n");
    fprintf(stderr, "   -v          verbose (print indirect block numbers)\n");
    fprintf(stderr, "   -d dev      device to read\n");
    fprintf(stderr, "   -n          ensure first address in block is after current block\n");
}


int main(int argc, char *argv[]) {
    // get file name
    int opt;
    char *drive_name = "/dev/sdb";
    int print_debug = 0;
    int ensure_address = 0;
    long block_break = -1;
    while ((opt = getopt(argc, argv, "d:hvnb:")) != -1) {
        switch (opt) {
            case 'd':
                drive_name = optarg;
                break;
            case 'h': // print help
                help(argv[0]);
                exit(EXIT_SUCCESS);
            case 'v': // print out debug printfs
                print_debug = 1;
                break;
            case 'n': // when checking for fake small indirect blocks assume first address in block is right after current block
                ensure_address = 1;
                break;
            case 'b': // when checking for fake small indirect blocks assume first address in block is right after current block
                block_break = strtol(optarg, NULL, 0);
                break;
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
    super_block_t super_block = readSuperBlock(drive_fd, part_address + 1024);
    check_part(super_block, drive_fd, part_address, print_debug, ensure_address, block_break);


    close(drive_fd);
    return EXIT_SUCCESS;
}

