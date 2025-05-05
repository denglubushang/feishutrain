#pragma once
#include <string.h>
#include <cstdint>
#include<string>
#include <openssl/md5.h>
#include <openssl/evp.h>
#define File_segdata_size 1024*511

#pragma pack(push, 1) // �����ڴ����
struct FileSeg {
	uint64_t datasize;  // �ļ����ݵĳ���
	uint64_t seg_id;	//�ļ������
	unsigned char hash[MD5_DIGEST_LENGTH];//hashֵ
	char filedata[File_segdata_size];//�ļ�����
};

struct logSeg {
	char password[1024];
	void init();
};

struct FileInformation {
	char header[1024];    // �ļ���
	uint64_t filesize;  // �ļ����ݵĳ���
};
#pragma pack(pop)      // �ָ�Ĭ�϶���

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
