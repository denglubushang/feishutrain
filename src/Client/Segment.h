#pragma once
#include <string.h>
#include <cstdint>
#include<string>
#include <fstream>
#include <vector>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/buffer.h>
#include <openssl/params.h>
#include <openssl/core_names.h>
#define File_segdata_size 1024*64
#define PassMaxlen 10
#pragma pack(push, 1) // 禁用内存对齐
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
	void init();
	int Set_datasize(uint64_t datsize);
	int Set_segid(uint64_t seq);
	int Set_hash();
	int Encrypt_data();  // 加密数据
};
class HeadSegment {
public:
	FileInformation information;
	void init();
	void Set_Boolean(bool is_continue);
	int Set_header(std::string filename);
	int Set_filesize(uint64_t size);
};
