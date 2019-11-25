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
#include <fstream>
#include <typeinfo>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <unordered_map>  
#include <unordered_set>
#include <set>
#include <bits/stdc++.h> 


using namespace std;

const char* localHostAddress = "127.0.0.1";

// embeded server side port number
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
    /*
        create UDP socket for server A:
    */
    // PF_INET or AF_INET?
    int serverSocketB = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocketB < 0) {
        cout<<"Error detected when creating the socket!"<<endl;
        close(serverSocketB);
        return 0;
    }

    /*
        bind the socket with a port, referred from Beej's
    */
    struct sockaddr_in server_B_addr;
    server_B_addr.sin_family = AF_INET;
    server_B_addr.sin_port = htons(PORT_SERVER_B_UDP);
    server_B_addr.sin_addr.s_addr = inet_addr(localHostAddress);
    if (bind(serverSocketB, (struct sockaddr *)&server_B_addr, sizeof server_B_addr) < 0) {
        cout<<"Error detected when binding the port!"<<endl;
        close(serverSocketB);
        return 0;
    }
    cout<<"The Server B is up and running using UDP on port <"<<PORT_SERVER_B_UDP<<">."<<endl;

    while (true) {
        /*
            receive shortest paths info from AWS
        */
        char* buffer_res = new char[MAX_DATA_SIZE];
        struct sockaddr_in storage_addr;            
        socklen_t fromlen = sizeof storage_addr; 
        if (recvfrom(serverSocketB, buffer_res, MAX_DATA_SIZE, 0, (struct sockaddr *)&storage_addr, &fromlen) < 0) {
            cout<<"Error occurred when receiving results info from AWS!"<<endl;
            close(serverSocketB);
            return 0;
        }
        char* p = buffer_res;

        long long int fileSize = -1;
        memcpy(&fileSize, p, sizeof(long long int));
        p += sizeof(long long int);
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

        for (int i = 0; i < res.size();i++) {
            memcpy (&res[i].index, p, sizeof(int));
            p += sizeof(int);
            memcpy (&res[i].numNode, p, sizeof(int));
            p += sizeof(int);
            memcpy (&res[i].distance, p, sizeof(int));
            p += sizeof(int);
        }
        cout << "The Server B has received data for calculation:"<<endl;
        cout << "* Propagation speed: <"<< vp <<"> km/s;" << endl;
        cout << "* Transmission speed: <"<< vt <<"> Bytes/s;" << endl;
        for (int i = 0; i < res.size();i++) {
            cout<<"* Path length for destination <"<< res[i].index <<">: <" << res[i].distance << ">;" <<endl;
        }

        vector<DelayInfo> delays(numVertice-1);
        for(int i=0; i<delays.size(); i++){
            DelayInfo& dInfo = delays[i];
            NodeInfo& nInfo = res[i];
            dInfo.distIndex = nInfo.index;
            dInfo.distance = nInfo.distance;
            // dInfo.Tt = (nInfo.numNode - 1) * (fileSize /(vt * 8)) * 1000;  // another way of calculation Tt
            dInfo.Tt = (fileSize /(vt * 8)) * 1000;
            dInfo.Tp = (nInfo.distance / vp) * 1000;
        }

        cout << "The Server B has finished the calculation of the delays:"<<endl;
        cout << "--------------------------------" << endl;
        cout << "Destination    Delay" << endl;
        cout << "--------------------------------" << endl;
        for (int i = 0; i < delays.size();i++) {
            cout<< left << setw(15) << delays[i].distIndex << setiosflags(ios::fixed) << setprecision(2) << delays[i].Tt + delays[i].Tp <<endl;
        }
        cout << "----------------------------" << endl;

        /*
            encoding result
        */
        char buffer_delay[MAX_DATA_SIZE];
        char* p2 = buffer_delay;
        for (int i = 0; i < delays.size();i++) {
            memcpy (p2, &delays[i].distIndex, sizeof(int));
            p2 += sizeof(int);
            memcpy (p2, &delays[i].distance, sizeof(int));
            p2 += sizeof(int);
            memcpy (p2, &delays[i].Tt, sizeof(double));
            p2 += sizeof(double);
            memcpy (p2, &delays[i].Tp, sizeof(double));
            p2 += sizeof(double);
        }
        
        /*
            send delay to AWS
        */
        if (sendto(serverSocketB, buffer_delay, p2 - buffer_delay, 0, (struct sockaddr *)&storage_addr, fromlen) < 0) {
            cout<<"Error occurred when sending delays to AWS."<<endl;
            close(serverSocketB);
            return 0;
        }
        cout<<"The Server B has finished sending the output to AWS"<<endl;
    }

    close(serverSocketB);
    return 0;
}
