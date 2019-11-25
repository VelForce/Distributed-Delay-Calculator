#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <iostream>
#include <typeinfo>
#include <vector>
#include <iomanip>
#include <sstream>

using namespace std;

const char* localHostAddress = "127.0.0.1";

// embeded server side port number
#define PORT_SERVER_A_UDP 21641
#define PORT_SERVER_B_UDP 22641
#define PORT_BACKEND_UDP 23641
#define PORT_CLIENT_TCP 24641

#define QUEUE_LIMIT 10
#define MAX_DATA_SIZE 10000

typedef struct NodeInfo {
    int index;
    int numNode; // number of node in the path
    int distance; // distance to source
}NodeInfo;

typedef struct {
    int distIndex; // distination node index
    int distance; // distance to source
    double Tt;
    double Tp;
}DelayInfo;

int main() {
    // =========================================================================================================
    // ----------------------------------------Client Socket-----------------------------------------------------
	/*
		create AWS client TCP socket for client communication
	*/
	int client_socket = socket(AF_INET, SOCK_STREAM, 0);		//referred from Beej's
	if (client_socket < 0) {
		cout<<"Error detected when creating the socket for client!"<<endl;
		close(client_socket);
		return 0;
	}
	/*
		bind the AWS client socket with a port, referred from Beej's
	*/
	struct sockaddr_in addr_for_client;
    addr_for_client.sin_family = AF_INET;
    addr_for_client.sin_port = htons(PORT_CLIENT_TCP);
    addr_for_client.sin_addr.s_addr = inet_addr(localHostAddress);
    if (bind(client_socket, (struct sockaddr *)&addr_for_client, sizeof addr_for_client) < 0) {
   		cout<<"Error detected when binding the port for client socket!"<<endl;
        close(client_socket);
        return 0;
    }
    // =========================================================================================================
    // ----------------------------------------Backend Server Socket--------------------------------------------
    /*
        create UDP socket for communication with back-servers
    */
    int backend_socket = socket(AF_INET, SOCK_DGRAM, 0);     //referred from Beej's
    if (backend_socket < 0) {
        cout<<"Error detected when creating the socket for backend server!"<<endl;
        close(backend_socket);
        return 0;
    }
    /*
        bind the socket with a port, referred from Beej's
    */
    struct sockaddr_in addr_for_server;
    addr_for_server.sin_family = AF_INET;
    addr_for_server.sin_port = htons(PORT_BACKEND_UDP);
    addr_for_server.sin_addr.s_addr = inet_addr(localHostAddress);
    int bindResultServer = bind(backend_socket, (struct sockaddr *)&addr_for_server, sizeof addr_for_server);
    if (bindResultServer < 0) {
        cout<<"Error detected when binding the port for backend server!"<<endl;
        close(backend_socket);
        return 0;
    }
    
    cout<<"The AWS is up and running."<<endl;

    // keep listening for incoming from client
    int listenClientResult = listen(client_socket, QUEUE_LIMIT);

    // =========================================================================================================
    // ---------------------------------------------------------------------------------------------------------

    while(true) {
        
        /*
            accept for the incoming client(s)!
        */
        socklen_t addr_for_client_size = sizeof addr_for_client;
        int childSocket_client = accept(client_socket, (struct sockaddr *)&addr_for_client, &addr_for_client_size);
        if (childSocket_client < 0) {
            cout<<"Error detected when accepting the connection with client!"<<endl;
            close(childSocket_client);
            return 0;
        }
        /*
            receive messages from the client over TCP
        */
        char* buffer_client = new char[MAX_DATA_SIZE];
        int recvResultClient = recv(childSocket_client, buffer_client, MAX_DATA_SIZE, 0);
        if (recvResultClient < 0) {
            cout<<"Error occurred when receiving message from client!"<<endl;
            close(childSocket_client);
            return 0;
        }
        string client_data = buffer_client;
        char *temp = new char[MAX_DATA_SIZE];
        strcpy(temp, buffer_client);
        // get arguments
        string mapId = strtok(temp, "/");
        int sourceIndex = atoi(strtok(NULL, "/"));
        long long int fileSize = atoll(strtok(NULL, "/"));

        cout << "The AWS has received map ID <"<< mapId <<">, start vertex <"<< sourceIndex <<"> and file size <"<< fileSize 
             <<"> from the client using TCP over port <"<< PORT_CLIENT_TCP <<">" << endl;
        // =========================================================================================================
        // ---------------------------------------------------------------------------------------------------------
        /*
            send messages to Server A
        */
        struct sockaddr_in server_A_addr;
        server_A_addr.sin_family = AF_INET;
        server_A_addr.sin_port = htons(PORT_SERVER_A_UDP);
        server_A_addr.sin_addr.s_addr = inet_addr(localHostAddress);
        socklen_t tolen_A = sizeof server_A_addr;
        if (sendto(backend_socket, client_data.c_str(), sizeof(client_data), 0, (struct sockaddr *)&server_A_addr, tolen_A) < 0) {
            cout<<"Error occurred when sending to Server A."<<endl;
            close(backend_socket);
            return 0;
        }
        cout<<"The AWS has sent map ID and starting vertex to server A using UDP over port <"<<PORT_BACKEND_UDP<<">"<<endl;
        /*
            recieve result from Server A
        */
        char buffer_res[MAX_DATA_SIZE];
        if (recvfrom(backend_socket, buffer_res, MAX_DATA_SIZE, 0, (struct sockaddr *)&server_A_addr, &tolen_A) < 0) {
            cout<<"Error occurred when receiving from Server A!"<<endl;
            close(backend_socket);
            return 0;
        }
        char* p = buffer_res;

        double vp = -1;
        memcpy(&vp, p, sizeof(double));
        p += sizeof(double);
        double vt = -1;
        memcpy(&vt, p, sizeof(double));
        p += sizeof(double);
        int numVertice = -1;
        memcpy(&numVertice, p, sizeof(int));
        p += sizeof(int);

        vector<NodeInfo> res(numVertice-1); // store shortest paths
        for (int i = 0; i < numVertice - 1;i++) {
            memcpy (&res[i].index, p, sizeof(int));
            p += sizeof(int);
            memcpy (&res[i].numNode, p, sizeof(int));
            p += sizeof(int);
            memcpy (&res[i].distance, p, sizeof(int));
            p += sizeof(int);
        }
        cout << "The AWS has received shortest path from server A:"<<endl;
        cout << "----------------------------" << endl;
        cout << "Destination    Min Length" << endl;
        cout << "----------------------------" << endl;
        for (int i = 0; i < res.size();i++) {
            cout<< left << setw(15) << res[i].index << res[i].distance <<endl;
        }
        cout << "----------------------------" << endl;

        char bufferWithFileSize[MAX_DATA_SIZE];
        char* p2 = bufferWithFileSize;
        memcpy (p2, &fileSize, sizeof(long long int));
        p2 += sizeof(long long int);
        memcpy (p2, &buffer_res, p - buffer_res);
        p2 += p - buffer_res;
        // =========================================================================================================
        // ---------------------------------------------------------------------------------------------------------
        /*
            send shortest paths to Server B
        */
        struct sockaddr_in server_B_addr;
        server_B_addr.sin_family = AF_INET;
        server_B_addr.sin_port = htons(PORT_SERVER_B_UDP);
        server_B_addr.sin_addr.s_addr = inet_addr(localHostAddress);
        socklen_t tolen_B = sizeof server_B_addr;
        if (sendto(backend_socket, bufferWithFileSize, p2 - bufferWithFileSize, 0, (struct sockaddr *)&server_B_addr, tolen_B) < 0) {
            cout<<"Error occurred when sending result to Server B."<<endl;
            close(backend_socket);
            return 0;
        }
        cout<<"The AWS has sent path length, propagation speed and transmission speed to server B using UDP over port <"<<PORT_BACKEND_UDP<<">"<<endl;

        /*
            recieve result from Server B
        */
        char buffer_delay[MAX_DATA_SIZE];
        if (recvfrom(backend_socket, buffer_delay, MAX_DATA_SIZE, 0, (struct sockaddr *)&server_B_addr, &tolen_B) < 0) {
            cout<<"Error occurred when receiving from Server B!"<<endl;
            close(backend_socket);
            return 0;
        }
        char* p3 = buffer_delay;
        vector<DelayInfo> delays(numVertice-1); // store shortest paths
        for (int i = 0; i < delays.size();i++) {
            memcpy (&delays[i].distIndex, p3, sizeof(int));
            p3 += sizeof(int);
            memcpy (&delays[i].distance, p3, sizeof(int));
            p3 += sizeof(int);
            memcpy (&delays[i].Tt, p3, sizeof(double));
            p3 += sizeof(double);
            memcpy (&delays[i].Tp, p3, sizeof(double));
            p3 += sizeof(double);
        }
        cout << "The AWS has received delays from server B:"<<endl;
        cout << "------------------------------------------------------------------------" << endl;
        cout << "Destination    Tt                  Tp                  Delay" << endl;
        cout << "------------------------------------------------------------------------" << endl;
        for (int i = 0; i < delays.size();i++) {
            cout<< left << setw(15) << delays[i].distIndex << left << setw(20) << setiosflags(ios::fixed) << setprecision(2) << delays[i].Tt
                << left << setw(20) << setiosflags(ios::fixed) << setprecision(2) << delays[i].Tp
                << left << setw(20) << setiosflags(ios::fixed) << setprecision(2) << delays[i].Tt + delays[i].Tp <<endl;
        }
        cout << "------------------------------------------------------------------------" << endl;
        // =========================================================================================================
        // ---------------------------------------------------------------------------------------------------------
        /*
            send delays to client using TCP
        */
        char bufferWithNumVertice[MAX_DATA_SIZE];
        char* p4 = bufferWithNumVertice;
        memcpy (p4, &numVertice, sizeof(int));
        p4 += sizeof(int);
        memcpy (p4, &buffer_delay, p3 - buffer_delay);
        p4 += p3 - buffer_delay;
        if (send(childSocket_client, bufferWithNumVertice, p4 - bufferWithNumVertice, 0) < 0) {
            cout<<"Error occurred when sending delays to client!"<<endl;
            close(childSocket_client);
            return 0;
        }
        cout << "The AWS has sent calculated delay to client using TCP over port <"<< PORT_CLIENT_TCP << ">."<<endl;
    }

    close(client_socket);
    close(backend_socket);
	return 0;
}