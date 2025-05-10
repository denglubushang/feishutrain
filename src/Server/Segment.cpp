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
	// ��ȡAES��Կ
	std::vector<unsigned char> aes_key, hmac_key;
	if (!read_keys(aes_key, hmac_key)) {
		std::cout << "��ȡ��Կʧ��" << std::endl;
		return -1;
	}

	// ������������Ļ�����
	unsigned char* decrypted_data = new unsigned char[data.datasize];

	// ��������������
	EVP_CIPHER* cipher = EVP_CIPHER_fetch(NULL, "AES-256-GCM", NULL);
	if (cipher == NULL) {
		delete[] decrypted_data;
		std::cout << "��ȡ�����㷨ʧ��" << std::endl;
		return -1;
	}

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL) {
		EVP_CIPHER_free(cipher);
		delete[] decrypted_data;
		std::cout << "��������������ʧ��" << std::endl;
		return -1;
	}

	int outlen, tmplen;

	// ��ʼ�����ܲ���
	if (EVP_DecryptInit_ex2(ctx, cipher, aes_key.data(), data.iv, NULL) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] decrypted_data;
		std::cout << "��ʼ������ʧ��" << std::endl;
		return -1;
	}

	// �ṩ��������(AAD)������еĻ�
	unsigned char aad[8];
	memcpy(aad, &data.seg_id, sizeof(data.seg_id));
	if (EVP_DecryptUpdate(ctx, NULL, &outlen, aad, sizeof(aad)) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] decrypted_data;
		std::cout << "����AADʧ��" << std::endl;
		return -1;
	}

	// ��������
	if (EVP_DecryptUpdate(ctx, decrypted_data, &outlen, reinterpret_cast<const unsigned char*>(data.filedata), static_cast<int>(data.datasize)) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] decrypted_data;
		std::cout << "��������ʧ��" << std::endl;
		return -1;
	}
	int decrypted_len = outlen;

	// ������������֤��ǩ
	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, sizeof(data.auth_tag), data.auth_tag) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		EVP_CIPHER_free(cipher);
		delete[] decrypted_data;
		std::cout << "������֤��ǩʧ��" << std::endl;
		return -1;
	}

	// ��ɽ��ܲ���֤
	int ret = EVP_DecryptFinal_ex(ctx, decrypted_data + outlen, &tmplen);

	if (ret > 0) {
		// ��֤�ɹ����滻��������Ϊ��������
		decrypted_len += tmplen;
		memcpy(data.filedata, decrypted_data, decrypted_len);
		data.datasize = decrypted_len;
	}
	else {
		// ��֤ʧ��
		std::cout << "AES-GCM��֤ʧ�ܣ����ݿ��ܱ��۸ģ�" << std::endl;
	}

	// ����
	delete[] decrypted_data;
	EVP_CIPHER_CTX_free(ctx);
	EVP_CIPHER_free(cipher);

	return ret > 0 ? 0 : -1;
}