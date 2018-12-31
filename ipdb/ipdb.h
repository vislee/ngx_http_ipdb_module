//
// Created by root on 11/14/18.
//

#ifndef IPDB_C_IPDB_H
#define IPDB_C_IPDB_H

#define  IPv4  0x01
#define  IPv6  0x02

#define ErrNoErr 0 //No error.
#define ErrFileSize 1 //"IP Database file size error."
#define ErrMetaData 2 //"IP Database metadata error."
//#define ErrReadFull 3 //"IP Database ReadFull error."

#define ErrDatabaseError 4 //"database error"

#define ErrIPFormat 5 //"Query IP Format error."

#define ErrNoSupportLanguage 6 //"language not support"
#define ErrNoSupportIPv4 7 //"IPv4 not support"
#define ErrNoSupportIPv6 8 //"IPv6 not support"

#define ErrDataNotExists 9 //"data is not exists"

typedef struct ipdb_meta_data_language {
    char name[8];
    int offset;
} ipdb_meta_data_language;

typedef struct ipdb_meta_data {
    int node_count;
    int total_size;
    short ip_version;
    long build_time;
    ipdb_meta_data_language *language;
    int language_length;
    char **fields;
    int fields_length;
} ipdb_meta_data;

typedef struct ipdb_reader {
    ipdb_meta_data *meta;
    int v4offset;
    int file_size;
    int data_size;
    unsigned char *data;
} ipdb_reader;

int ipdb_reader_new(const char *file, ipdb_reader **reader);

void ipdb_reader_free(ipdb_reader **reader);

int ipdb_reader_is_ipv4_support(ipdb_reader *reader);

int ipdb_reader_is_ipv6_support(ipdb_reader *reader);

int ipdb_reader_find(ipdb_reader *reader, const char *addr, const char *language, char *body);

#endif //IPDB_C_IPDB_H
