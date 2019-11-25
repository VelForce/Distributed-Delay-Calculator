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
#include <iomanip>
#include <vector>

using namespace std;

const char* localHostAddress = "127.0.0.1";

// attention: embed the port number for aws server into the client side!
#define PORT_CLIENT_TCP 24641

#define MAX_DATA_SIZE 10000

typedef struct {
    int distIndex; // distination node index
    int distance; // distance to source
    double Tt;
    double Tp;
}DelayInfo;

int main(int argc, char* argv[]) {
	if (argc < 4) {
		cout<<"Error! please enter the input with correct format!"<<endl;
		return 0;
	}
	/*
		create TCP socket for client:
	*/
	int clientSocket = socket(AF_INET, SOCK_STREAM, 0);		//referred from Beej's
	if (clientSocket < 0) {
		cout<<"Error detected when creating the socket!"<<endl;
		close(clientSocket);
		return 0;
	}
	/*
		connect to AWS server, referred from Beej's
	*/
	struct sockaddr_in aws_addr;
	aws_addr.sin_family = AF_INET;
	aws_addr.sin_addr.s_addr = inet_addr(localHostAddress);
	aws_addr.sin_port = htons(PORT_CLIENT_TCP);
	int connectResult = connect(clientSocket, (struct sockaddr *)&aws_addr, sizeof aws_addr);
	if (connectResult < 0) {
		cout<<"Error detected when binding the port!"<<endl;
		close(clientSocket);
		return 0;
	}

	cout<<"The client is up and running."<<endl;

	/*
		client send messages to AWS server over TCP
	*/
	string client_data = "";
	for (int i = 0; i < 3; i++) {
			client_data = client_data + argv[i + 1] + "/";
	}
	send(clientSocket, client_data.c_str(), sizeof(client_data), 0); 
    cout << "The client has sent query to AWS using TCP: "
         << "start vertex <"<< argv[2] <<">; map <"<< argv[1] <<">; file size <"<<  argv[3] <<">." << endl;
	/*
	    receive results from aws over TCP
	*/
	char buffer_delay[MAX_DATA_SIZE];
    if (recv(clientSocket, buffer_delay, MAX_DATA_SIZE, 0) < 0) {
        cout<<"Error occurred when receiving delays from AWS!"<<endl;
        close(clientSocket);
        return 0;
    }
    char* p3 = buffer_delay;
    int numVertice = -1;
    memcpy (&numVertice, p3, sizeof(int));
    p3 += sizeof(int);
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
    cout << "The client has received results from AWS:"<<endl;
    cout << "--------------------------------------------------------------------------------------" << endl;
    cout << "Destination    Min Length     Tt                  Tp                  Delay" << endl;
    cout << "--------------------------------------------------------------------------------------" << endl;
    for(int i = 0; i < delays.size();i++){
        cout << left << setw(15) << delays[i].distIndex << left << setw(15) << delays[i].distance;
        cout << left << setw(20) << setiosflags(ios::fixed) << setprecision(2) << delays[i].Tt;
        cout << left << setw(20) << setiosflags(ios::fixed) << setprecision(2) << delays[i].Tp;
        cout << left << setiosflags(ios::fixed) << setprecision(2) << delays[i].Tt + delays[i].Tp << endl;
    }
    cout << "--------------------------------------------------------------------------------------" << endl;

	close(clientSocket);
	return 0;
		
}