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
