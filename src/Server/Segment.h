#pragma once
#include <string.h>
#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/sha.h>  
#define FileNameBegin 0
#define SeqNum 1024
#define PassMaxlen 10
#define File_segdata_size 1024*511
#pragma pack(push, 1) // 禁用内存对齐

bool read_keys(std::vector<unsigned char>& aes_key, std::vector<unsigned char>& hmac_key);


struct FileSeg {
	uint64_t datasize;  // 文件数据的长度
	uint64_t seg_id;	//文件块序号
	unsigned char hash[SHA256_DIGEST_LENGTH];//hash值，改为SHA256长度
	unsigned char iv[12];        // AES-GCM初始化向量
	unsigned char auth_tag[16];  // AES-GCM认证标签
	char filedata[File_segdata_size];//文件数据
};

struct logSeg {
	char password[PassMaxlen];
	void init();
};

struct FileInformation {
	bool is_continue = false;     //续传标识,默认否
	char header[1024];    // 文件名
	uint64_t filesize;  // 文件数据的长度
};
#pragma pack(pop)      // 恢复默认对齐

class DataSegment
{
public:
	FileSeg data;
public:
	void init();
	int Set_datasize(uint64_t datsize);
	int Set_hash();
	int Set_filedata();
	int Decrypt_data();  // 解密数据
};
class HeadSegment {
public:
	FileInformation information;
public:
	void init();
	int Set_header(std::string filename);
	int Set_filesize(uint64_t size);
};
