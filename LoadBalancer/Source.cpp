//#include <ws2tcpip.h>
#include <WS2tcpip.h>

#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define MAX_CONNECTIONS 10

bool InitializeWindowsSockets();

void remove_socket(SOCKET* array, int index, int* array_len)
{
	for (int i = index; i < *array_len - 1; i++)
	{
		array[i] = array[i + 1];
	}
	array[*array_len - 1] = INVALID_SOCKET;

	*array_len -= 1;
}

void add_socket(SOCKET* array, SOCKET listenSocket, int* array_len)
{
	array[*array_len] = accept(listenSocket, nullptr, nullptr);
	if (array[*array_len] == INVALID_SOCKET)
	{
		printf("failed to accept new connection\n");
	}
	else
	{
		unsigned long mode = 1;
		if (ioctlsocket(array[*array_len], FIONBIO, &mode) != 0)
		{
			printf("failed to put socket in nonblocking mode\n");
			return;
		}
		else
		{
			*array_len += 1;
			printf("connection added, number: %d\n", *array_len);
		}
	}
}

void close_all_sockets(SOCKET* array, int* array_len)
{
	for (int i = 0; i < *array_len; i++)
	{
		closesocket(array[i]);
		array[i] = INVALID_SOCKET;
	}

	*array_len = 0;
}

//void recieve_mssg(SOCKET socket, char* recvbuf)
//{
//	int iResult;
//	iResult = recv(socket, recvbuf, DEFAULT_BUFLEN, 0);
//
//	if (iResult > 0)
//	{
//		printf("Message received from client: %s.\n", recvbuf);
//	}
//	else if (iResult == 0)
//	{
//		printf("closing connection with client %d", i);
//		remove_socket(socket, i, &currentConnections);
//
//	}
//	else
//	{
//		printf("recv on connection %d failed, closing socket", i);
//		closesocket(sock);
//		remove_socket(acceptedSocket, i, &currentConnections);
//	}
//}

int  main(void)
{
	// Socket used for listening for new clients 
	SOCKET listenSocket = INVALID_SOCKET;
	// Socket used for communication with client
	SOCKET acceptedSocket[MAX_CONNECTIONS];
	for (int i = 0; i < MAX_CONNECTIONS; i++)
	{
		acceptedSocket[i] = INVALID_SOCKET;
	}

	int currentConnections = 0;

	// variable used to store function return value
	int iResult;
	// Buffer used for storing incoming data
	char recvbuf[DEFAULT_BUFLEN];

	if (InitializeWindowsSockets() == false)
	{
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
	}

	// Prepare address information structures
	addrinfo* resultingAddress = NULL;
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;       // IPv4 address
	hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
	hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
	hints.ai_flags = AI_PASSIVE;     // 

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	listenSocket = socket(AF_INET,      // IPv4 address famly
		SOCK_STREAM,  // stream socket
		IPPROTO_TCP); // TCP

	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket - bind port number and local address 
	// to socket
	iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// Since we don't need resultingAddress any more, free it
	freeaddrinfo(resultingAddress);




	// Set listenSocket in listening mode
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	unsigned long mode = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &mode);
	if (iResult != NO_ERROR)
	{
		printf("ioctlsocket failed with error: %ld\n", iResult);
	}

	printf("Server initialized, waiting for clients.\n");


	fd_set readfds;

	timeval time_val;
	time_val.tv_sec = 1;
	time_val.tv_usec = 0;

	do
	{
		FD_ZERO(&readfds);

		if (currentConnections != MAX_CONNECTIONS)
		{
			FD_SET(listenSocket, &readfds);
		}

		for (int i = 0; i < currentConnections; i++)
		{
			FD_SET(acceptedSocket[i], &readfds);
		}

		int select_result = select(0, &readfds, nullptr, nullptr, &time_val);

		if (select_result == SOCKET_ERROR)
		{
			printf("error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			/*for (int i = 0; i < currentConnections; i++)
			{
				closesocket(acceptedSocket[i]);
			}*/
			close_all_sockets(acceptedSocket, &currentConnections);
			WSACleanup();

			return -2;
		}
		else if (select_result == 0)
		{
			printf("no events...\n");
		}

		if (FD_ISSET(listenSocket, &readfds))
		{
			add_socket(acceptedSocket, listenSocket, &currentConnections);
		}

		for (int i = 0; i < currentConnections; i++)
		{

			if (FD_ISSET(acceptedSocket[i], &readfds))
			{
				//recieve_mssg(acceptedSocket[i]);
				iResult = recv(acceptedSocket[i], recvbuf, DEFAULT_BUFLEN, 0);

				if (iResult > 0)
				{
					printf("Message received from client: %s.\n", recvbuf);
				}
				else if (iResult == 0)
				{
					printf("closing connection with client %d", i);
					remove_socket(acceptedSocket, i, &currentConnections);

				}
				else
				{
					printf("recv on connection %d failed, closing socket", i);
					closesocket(acceptedSocket[i]);
					remove_socket(acceptedSocket, i, &currentConnections);
				}
			}
		}


	} while (1);

	// shutdown the connection since we're done

	for (int i = 0; i < currentConnections; i++)
	{
		iResult = shutdown(acceptedSocket[i], SD_SEND);
		if (iResult == SOCKET_ERROR)
		{
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket[i]);
			WSACleanup();
			return 1;
		}
	}


	// cleanup
	closesocket(listenSocket);
	//for (int i = 0; i < currentConnections; i++)
	//{
	//	closesocket(acceptedSocket[i]);
	//}
	close_all_sockets(acceptedSocket, &currentConnections);
	WSACleanup();

	return 0;
}

bool InitializeWindowsSockets()
{
	WSADATA wsaData;
	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}