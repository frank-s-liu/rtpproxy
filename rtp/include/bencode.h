#ifndef RTPPROXY_BENCODE_H__
#define RTPPROXY_BENCODE_H__

typedef enum bencode_error
{
    SUCCESS = 0,
    FORMAT_ERR,
    TOO_LONG_STR,
    BENCODE_NOT_COMPLETED,
    BENCODE_SPLICE,
    BENCODE_ERRNO_MAX
}BENCODE_ERRNO;

int parseBencodeCmd(char* cmdstr);
int parsingString(char* bencode_str_start, char** bencode_str_end);
int parsingInt(char* bencode_str_start, char** bencode_str_end);
int bencodeCheck(char* cmdstr, char** end);

#endif
