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
#define File_segdata_size 1024*64
#pragma pack(push, 1) // �����ڴ����

bool read_keys(std::vector<unsigned char>& aes_key, std::vector<unsigned char>& hmac_key);


struct FileSeg {
	uint64_t datasize;  // �ļ����ݵĳ���
	uint64_t seg_id;	//�ļ������
	unsigned char hash[SHA256_DIGEST_LENGTH];//hashֵ����ΪSHA256����
	unsigned char iv[12];        // AES-GCM��ʼ������
	unsigned char auth_tag[16];  // AES-GCM��֤��ǩ
	char filedata[File_segdata_size];//�ļ�����
};

struct logSeg {
	char password[PassMaxlen];
	void init();
};

struct FileInformation {
	bool is_continue = false;     //������ʶ,Ĭ�Ϸ�
	char header[1024];    // �ļ���
	uint64_t filesize;  // �ļ����ݵĳ���
};
#pragma pack(pop)      // �ָ�Ĭ�϶���

class DataSegment
{
public:
	FileSeg data;
public:
	void init();
	int Set_datasize(uint64_t datsize);
	int Set_hash();
	int Set_filedata();
	int Decrypt_data();  // ��������
};
class HeadSegment {
public:
	FileInformation information;
public:
	void init();
	int Set_header(std::string filename);
	int Set_filesize(uint64_t size);
};
