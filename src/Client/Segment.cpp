#include "Segment.h"
void HeadSegment::init()
{
	memset(&information, '\0', sizeof(information));
}

int HeadSegment::Set_header(std::string filename)
{
	strcpy_s(information.header, filename.c_str());
	return 0;
}

int HeadSegment::Set_filesize(uint64_t size)
{
	information.filesize = size;
	return 0;
}

void DataSegment::init()
{
	memset(&data, '\0', sizeof(data));
}

int DataSegment::Set_datasize(uint64_t datsize)
{
	data.datasize = datsize;
	return 0;
}

int DataSegment::Set_segid(uint64_t seq)
{
	data.seg_id = seq;
	return 0;
}

int DataSegment::Set_hash()
{
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	const EVP_MD* md = EVP_md5();
	unsigned int md_len = MD5_DIGEST_LENGTH;
	EVP_DigestInit_ex(ctx, md, NULL);
	EVP_DigestUpdate(ctx, data.filedata, static_cast<size_t>(data.datasize));
	EVP_DigestFinal_ex(ctx, data.hash, &md_len);
	EVP_MD_CTX_free(ctx);
	return 0;
}

void logSeg::init()
{
	memset(&password, '\0', sizeof(password));
}
