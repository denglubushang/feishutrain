#pragma once
#include <string.h>
#include <cstdint>
#include<string>
#include <openssl/md5.h>
#include <openssl/evp.h>
#define File_segdata_size 1024*511

#pragma pack(push, 1) // 禁用内存对齐
struct FileSeg {
	uint64_t datasize;  // 文件数据的长度
	uint64_t seg_id;	//文件块序号
	unsigned char hash[MD5_DIGEST_LENGTH];//hash值
	char filedata[File_segdata_size];//文件数据
};

struct logSeg {
	char password[1024];
	void init();
};

struct FileInformation {
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

};
class HeadSegment {
public:
	FileInformation information;
	void init();
	int Set_header(std::string filename);
	int Set_filesize(uint64_t size);
};
