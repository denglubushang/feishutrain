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
#pragma pack(push, 1) // �����ڴ����
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
	void init();
	int Set_datasize(uint64_t datsize);
	int Set_segid(uint64_t seq);
	int Set_hash();
	int Encrypt_data();  // ��������
};
class HeadSegment {
public:
	FileInformation information;
	void init();
	void Set_Boolean(bool is_continue);
	int Set_header(std::string filename);
	int Set_filesize(uint64_t size);
};
