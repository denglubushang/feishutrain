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

int DataSegment::Set_filedata()
{
	return 0;
}

void logSeg::init()
{
	memset(&password, '\0', PassMaxlen);
}

int DataSegment::Decrypt_data()
{
	// 读取AES密钥
	std::vector<unsigned char> aes_key, hmac_key;
	if (!read_keys(aes_key, hmac_key)) {
		std::cout << "读取密钥失败" << std::endl;
		return -1;
	}

	// 创建解密所需的缓冲区
	unsigned char* decrypted_data = new unsigned char[data.datasize];

	// 创建解密上下文
	EVP_CIPHER* cipher = EVP_CIPHER_fetch(NULL, "AES-256-GCM", NULL);
	if (cipher == NULL) {
		delete[] decrypted_data;
		std::cout << "获取解密算法失败" << std::endl;
		return -1;
	}

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL) {
		EVP_CIPHER_free(cipher);
		delete[] decrypted_data;
		std::cout << "创建解密上下文失败" << std::endl;
		return -1;
	}

	int outlen, tmplen;

	// 初始化解密操作
	if (EVP_DecryptInit_ex2(ctx, cipher, aes_key.data(), data.iv, NULL) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] decrypted_data;
		std::cout << "初始化解密失败" << std::endl;
		return -1;
	}

	// 提供关联数据(AAD)，如果有的话
	unsigned char aad[8];
	memcpy(aad, &data.seg_id, sizeof(data.seg_id));
	if (EVP_DecryptUpdate(ctx, NULL, &outlen, aad, sizeof(aad)) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] decrypted_data;
		std::cout << "更新AAD失败" << std::endl;
		return -1;
	}

	// 解密数据
	if (EVP_DecryptUpdate(ctx, decrypted_data, &outlen, reinterpret_cast<const unsigned char*>(data.filedata), static_cast<int>(data.datasize)) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] decrypted_data;
		std::cout << "解密数据失败" << std::endl;
		return -1;
	}
	int decrypted_len = outlen;

	// 设置期望的认证标签
	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, sizeof(data.auth_tag), data.auth_tag) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] decrypted_data;
		std::cout << "设置认证标签失败" << std::endl;
		return -1;
	}

	// 完成解密并验证
	int ret = EVP_DecryptFinal_ex(ctx, decrypted_data + outlen, &tmplen);

	if (ret > 0) {
		// 认证成功，替换加密数据为解密数据
		decrypted_len += tmplen;
		memcpy(data.filedata, decrypted_data, decrypted_len);
		data.datasize = decrypted_len;
	}
	else {
		// 认证失败
		std::cout << "AES-GCM认证失败，数据可能被篡改！" << std::endl;
	}

	// 清理
	delete[] decrypted_data;
	EVP_CIPHER_CTX_free(ctx);
	EVP_CIPHER_free(cipher);

	return ret > 0 ? 0 : -1;
}