#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <winsock2.h>

void sendFile(SOCKET sock, const char* filename);

#endif // FILE_TRANSFER_H
