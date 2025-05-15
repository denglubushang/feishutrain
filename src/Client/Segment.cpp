#include "Segment.h"
void HeadSegment::init()
{
	memset(&information, '\0', sizeof(information));
}

void HeadSegment::Set_Boolean(bool is_continue) {
	information.is_continue = is_continue;
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

//Base64解码函数
std::vector<unsigned char> base64_decode(const std::string& input) {
	BIO* bio, * b64;

	int decodeLen = input.size();
	std::vector<unsigned char> buffer(decodeLen);

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new_mem_buf(input.c_str(), input.length());
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

	int len = BIO_read(bio, buffer.data(), decodeLen);
	buffer.resize(len);

	BIO_free_all(bio);
	return buffer;
}

bool read_keys(std::vector<unsigned char>& aes_key, std::vector<unsigned char>& hmac_key) {
	std::ifstream keyfile("key.txt");
	if (!keyfile.is_open()) {
		std::cout << "无法打开密钥文件" << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(keyfile, line)) {
		if (line.find("AES_KEY=") == 0) {
			std::string key_b64 = line.substr(8); // 跳过"AES_KEY="
			aes_key = base64_decode(key_b64);
		}
		else if (line.find("HMAC_KEY=") == 0) {
			std::string key_b64 = line.substr(9); // 跳过"HMAC_KEY="
			hmac_key = base64_decode(key_b64);
		}
	}

	keyfile.close();
	return (!aes_key.empty() && !hmac_key.empty());
}

int DataSegment::Set_hash()
{
	// 读取HMAC密钥
	std::vector<unsigned char> aes_key, hmac_key;
	if (!read_keys(aes_key, hmac_key)) {
		std::cout << "读取密钥失败" << std::endl;
		return -1;
	}

	// 使用EVP接口计算HMAC-SHA256
	EVP_MAC* mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
	if (mac == NULL) {
		std::cout << "获取HMAC失败" << std::endl;
		return -1;
	}

	EVP_MAC_CTX* ctx = EVP_MAC_CTX_new(mac);
	if (ctx == NULL) {
		EVP_MAC_free(mac);
		std::cout << "创建HMAC上下文失败" << std::endl;
		return -1;
	}

	OSSL_PARAM params[2];
	params[0] = OSSL_PARAM_construct_utf8_string("digest", const_cast<char*>("SHA256"), 0);
	params[1] = OSSL_PARAM_construct_end();

	if (EVP_MAC_init(ctx, hmac_key.data(), hmac_key.size(), params) != 1) {
		EVP_MAC_CTX_free(ctx);
		EVP_MAC_free(mac);
		std::cout << "初始化HMAC失败" << std::endl;
		return -1;
	}

	if (EVP_MAC_update(ctx, reinterpret_cast<const unsigned char*>(data.filedata), static_cast<size_t>(data.datasize)) != 1) {
		EVP_MAC_CTX_free(ctx);
		EVP_MAC_free(mac);
		std::cout << "更新HMAC失败" << std::endl;
		return -1;
	}

	size_t out_len = SHA256_DIGEST_LENGTH;
	if (EVP_MAC_final(ctx, data.hash, &out_len, SHA256_DIGEST_LENGTH) != 1) {
		EVP_MAC_CTX_free(ctx);
		EVP_MAC_free(mac);
		std::cout << "完成HMAC失败" << std::endl;
		return -1;
	}

	EVP_MAC_CTX_free(ctx);
	EVP_MAC_free(mac);

	return 0;
}

// 新增AES-GCM加密方法
int DataSegment::Encrypt_data()
{
	// 读取AES密钥
	std::vector<unsigned char> aes_key, hmac_key;
	if (!read_keys(aes_key, hmac_key)) {
		std::cout << "读取密钥失败" << std::endl;
		return -1;
	}

	// 创建加密所需的缓冲区
	unsigned char* encrypted_data = new unsigned char[data.datasize + EVP_MAX_BLOCK_LENGTH];

	// 生成随机IV
	RAND_bytes(data.iv, sizeof(data.iv));

	// 创建加密上下文
	EVP_CIPHER* cipher = EVP_CIPHER_fetch(NULL, "AES-256-GCM", NULL);
	if (cipher == NULL) {
		delete[] encrypted_data;
		std::cout << "获取加密算法失败" << std::endl;
		return -1;
	}

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL) {
		EVP_CIPHER_free(cipher);
		delete[] encrypted_data;
		std::cout << "创建加密上下文失败" << std::endl;
		return -1;
	}

	int outlen, tmplen;

	// 初始化加密操作
	if (EVP_EncryptInit_ex2(ctx, cipher, aes_key.data(), data.iv, NULL) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] encrypted_data;
		std::cout << "初始化加密失败" << std::endl;
		return -1;
	}

	// 提供关联数据(AAD)，如果有的话
	unsigned char aad[8];
	memcpy(aad, &data.seg_id, sizeof(data.seg_id));
	if (EVP_EncryptUpdate(ctx, NULL, &outlen, aad, sizeof(aad)) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] encrypted_data;
		std::cout << "更新AAD失败" << std::endl;
		return -1;
	}

	// 加密数据
	if (EVP_EncryptUpdate(ctx, encrypted_data, &outlen, reinterpret_cast<const unsigned char*>(data.filedata), static_cast<int>(data.datasize)) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] encrypted_data;
		std::cout << "加密数据失败" << std::endl;
		return -1;
	}
	int encrypted_len = outlen;

	// 完成加密
	if (EVP_EncryptFinal_ex(ctx, encrypted_data + outlen, &tmplen) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] encrypted_data;
		std::cout << "完成加密失败" << std::endl;
		return -1;
	}
	encrypted_len += tmplen;

	// 获取认证标签
	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, sizeof(data.auth_tag), data.auth_tag) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] encrypted_data;
		std::cout << "获取认证标签失败" << std::endl;
		return -1;
	}

	// 替换原始数据为加密数据
	memcpy(data.filedata, encrypted_data, encrypted_len);
	data.datasize = encrypted_len;

	// 清理
	delete[] encrypted_data;
	EVP_CIPHER_CTX_free(ctx);
	EVP_CIPHER_free(cipher);

	return 0;
}

void logSeg::init()
{
	memset(&password, '\0', PassMaxlen);
}
