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
#define PORT_SERVER_A_UDP 21641
#define PORT_BACKEND_UDP 23641
#define PORT_CLIENT_TCP 24641

#define QUEUE_LIMIT 10
#define MAX_DATA_SIZE 10000

typedef struct {
    string id;  // map id
    double vp; // propagation speed
    double vt; // transmission speed
    int numVertice;
    int numEdge;
    unordered_map<int, vector<pair<int, int>>> graph;
}MapInfo;

typedef struct NodeInfo {
    int index;
    int numNode; // number of node in the path
    int distance; // distance to source
}NodeInfo;

struct cmpByDistance {
    bool operator()(const NodeInfo &a, const NodeInfo& b) const {
        if(a.distance != b.distance) {
            return a.distance < b.distance;
        } else {
            return a.index < b.index;
        }
    }
};

struct cmpByLetter {
    bool operator() (const MapInfo& a, const MapInfo& b) const {
        return a.id.compare(b.id) < 0;
    }
};

struct cmpByIndex {
    bool operator() (const NodeInfo& a, const NodeInfo& b) const {
        return (a.index < b.index);
    }
};

int main() {
	/*
		create UDP socket for server A:
	*/
	// PF_INET or AF_INET?
	int serverSocketA = socket(AF_INET, SOCK_DGRAM, 0);		//referred from Beej's
	if (serverSocketA < 0) {
		cout<<"Error detected when creating the socket!"<<endl;
		close(serverSocketA);
		return 0;
	}

	/*
		bind the socket with a port, referred from Beej's
	*/
	struct sockaddr_in server_A_addr;
    server_A_addr.sin_family = AF_INET;
    server_A_addr.sin_port = htons(PORT_SERVER_A_UDP);
    server_A_addr.sin_addr.s_addr = inet_addr(localHostAddress);
	int bindResult = bind(serverSocketA, (struct sockaddr *)&server_A_addr, sizeof server_A_addr);
    if (bindResult < 0) {
   		cout<<"Error detected when binding the port!"<<endl;
        close(serverSocketA);
        return 0;
    }
    cout<<"The Server A is up and running using UDP on port <"<<PORT_SERVER_A_UDP<<">."<<endl;

    /*
        build map
    */
    unordered_map<string, MapInfo> maps;
    MapInfo tempMap;
    ifstream infile; 
    infile.open("map.txt");   // open file
    assert(infile.is_open());   // when open failed

    string s = "";
    getline(infile,s);
    s.erase(s.size() - 1);
    tempMap.id = s;
    getline(infile,s);
    tempMap.vp = stof(s);
    getline(infile,s);
    tempMap.vt = stof(s);

    int countVertice = 0;
    int countEdge = 0;
    while(getline(infile,s)){
        if(isalpha(s[0])){
            tempMap.numVertice = countVertice;
            tempMap.numEdge = countEdge;
            maps[tempMap.id] = tempMap;
            countVertice = 0;
            countEdge = 0;
            tempMap = MapInfo();
            s.erase(s.size() - 1);
            tempMap.id = s;
            getline(infile,s);
            tempMap.vp = stof(s);
            getline(infile,s);
            tempMap.vt = stof(s);
        }else{
            char* chars = (char*)s.c_str();
            int node1 = atoi(strtok(chars, " "));
            int node2 = atoi(strtok(NULL, " "));
            int edgeVal = atoi(strtok(NULL, " "));
            // add edge to node1
            if(tempMap.graph.find(node1) == tempMap.graph.end()){
                tempMap.graph[node1] = vector<pair<int, int>>();
                countVertice++;
            }
            tempMap.graph.find(node1)->second.push_back(make_pair(node2, edgeVal));
            // add edge to node2
            if(tempMap.graph.find(node2) == tempMap.graph.end()){
                tempMap.graph[node2] = vector<pair<int, int>>();
                countVertice++;
            }
            tempMap.graph.find(node2)->second.push_back(make_pair(node1, edgeVal));
            countEdge++;
        }
    }
    tempMap.numVertice = countVertice;
    tempMap.numEdge = countEdge;
    maps[tempMap.id] = tempMap;

    infile.close();  //close file 

    // sort the maps by letter order
    vector<MapInfo> mapInfoList;
    for ( auto it = maps.begin(); it != maps.end(); it++ ) {
        MapInfo& info = it->second;
        mapInfoList.push_back(info);
    }
    sort(mapInfoList.begin(), mapInfoList.end(), cmpByLetter());

    cout<<"The Server A has constructed a list of <"<<maps.size()<<"> maps:"<<endl;
    cout<<"------------------------------------------"<<endl;
    cout<<"Map ID    Num Vertices   Num Edges"<<endl;
    cout<<"------------------------------------------"<<endl;
    for (int i=0; i<mapInfoList.size(); i++) {
        MapInfo& info = mapInfoList[i];
        cout << left << setw(10) << info.id << left << setw(15) << info.numVertice << left << setw(15) << info.numEdge << endl;
    }
    cout<<"------------------------------------------"<<endl;

    // debug map structure
    // for ( auto it = maps.begin(); it != maps.end(); it++ ) {
    //     unordered_map<int, vector<pair<int, int>>>& tempGraphInfo = (it->second).graph;
    //     cout << it->first << endl;
    //     for (auto i = tempGraphInfo.begin(); i != tempGraphInfo.end(); i++) {
    //         cout << i->first << "       " << endl; 
    //         for (const auto j: i->second) {
    //             cout << j.first << ", " << j.second << endl;
    //         }
    //     }
    // }

    while (true) {
    	/*
			receive arguments info from AWS
    	*/
        char* buffer_client = new char[MAX_DATA_SIZE];
    	struct sockaddr_in aws_addr;            
    	socklen_t fromlen = sizeof aws_addr; 
        int recvResult = recvfrom(serverSocketA, buffer_client, MAX_DATA_SIZE, 0, (struct sockaddr *)&aws_addr, &fromlen);
        if (recvResult < 0) {
    		cout<<"Error occurred when receiving link info from AWS!"<<endl;
    		close(serverSocketA);
    		return 0;
    	}
        // get arguments
        string client_data = buffer_client;
        char *temp = new char[MAX_DATA_SIZE];
        strcpy(temp, buffer_client);
        string mapId = strtok(temp, "/");
        int sourceIndex = atoi(strtok(NULL, "/"));
        long long int fileSize = atoll(strtok(NULL, "/"));

        cout << "The Server A has received input for finding shortest paths: starting vertex <"<<sourceIndex<<"> of map <"<<mapId<<">." << endl;

        /*
            find shortest path
        */
        set<NodeInfo, cmpByDistance> pq;
        unordered_map<int, NodeInfo> nodeInfoMap;
        unordered_set<int> visited;
        unordered_map<int, vector<pair<int, int>>>& graph = maps[mapId].graph;
        vector<NodeInfo> res;

        for ( auto it = graph.begin(); it != graph.end(); it++ ) {
            int index = it->first;
            NodeInfo node = {index, 0, INT_MAX};
            nodeInfoMap[index] = node;
        }

        NodeInfo& source = nodeInfoMap[sourceIndex];
        source.distance = 0;
        source.numNode = 1;
        pq.insert(source);
        while(!pq.empty()){
            auto nearestNodeIt = pq.begin();
            NodeInfo nodeCopy = *nearestNodeIt;
            pq.erase(nearestNodeIt);
            visited.insert(nodeCopy.index);
            // debug code
            // cout << "node index: " << nodeCopy.index << endl;
            // for ( auto it = visited.begin(); it != visited.end(); it++ ) {
            //     cout <<"visited: "<< *it << endl;
            // }
            res.push_back(nodeCopy);
            vector<pair<int, int>>& neighbors = graph[nodeCopy.index];
            // cout << "node index: " << nodeCopy.index << " size: "<< neighbors.size() << endl; // debug code
            int distToNewNode = nodeCopy.distance;
            int pathNodeNum = nodeCopy.numNode;
            // cout << "ok1: " <<endl;
            for(int i=0; i<neighbors.size(); i++){
                int nIndex = neighbors[i].first;
                int edge = neighbors[i].second;
                // cout << "ok2: "<< nIndex << " size: "<< neighbors.size() <<endl; // debug code
                if(visited.find(nIndex) == visited.end()){
                    // cout << "ok3: " <<endl;
                    int oldDist = nodeInfoMap[nIndex].distance;
                    int newDist = distToNewNode + edge;
                    // cout <<"                 neighbor node index : " << nIndex << " new dist :" << newDist <<endl; // debug code
                    if(newDist < oldDist){
                        NodeInfo& nextNode = nodeInfoMap[nIndex];
                        // cout << "                               nextNode index: "<< nextNode.index <<endl; // debug code
                        auto pqIt = pq.find(nextNode);
                        if(pqIt != pq.end()){
                            pq.erase(pqIt);
                        }
                        nextNode.distance = newDist;
                        // cout << "ok3: "<< nIndex <<endl; // debug code
                        nextNode.numNode = pathNodeNum + 1;
                        // cout << "ok4: "<< nIndex <<endl; // debug code
                        pq.insert(nextNode);
                        // cout << "ok5: "<< nIndex <<endl; // debug code
                    }
                }
            }
        }
        res.erase(res.begin()); // remove source node
        sort(res.begin(), res.end(), cmpByIndex()); // sort the result by node index in increasing order

        /*
            Print shortest path results
        */
        cout << "The Server A has identified the following shortest paths:" << endl;
        cout << "------------------------------" << endl;
        cout << "Destination    Min Length" << endl;
        cout << "------------------------------" << endl;
        for (int i = 0; i < res.size();i++) {
            cout<< left << setw(15) << res[i].index << res[i].distance <<endl;
        }
        cout << "------------------------------" << endl;

        /*
            encoding result
        */
        MapInfo& curMap = maps[mapId];
        char buffer_result[MAX_DATA_SIZE];
        char* start = buffer_result;
        char* p = buffer_result;
        memcpy (p, &curMap.vp, sizeof(double));
        p += sizeof(double);
        memcpy (p, &curMap.vt, sizeof(double));
        p += sizeof(double);
        memcpy (p, &curMap.numVertice, sizeof(int));
        p += sizeof(int);
        for (int i = 0; i < res.size();i++) {
            memcpy (p, &res[i].index, sizeof(int));
            p += sizeof(int);
            memcpy (p, &res[i].numNode, sizeof(int));
            p += sizeof(int);
            memcpy (p, &res[i].distance, sizeof(int));
            p += sizeof(int);
        }

        /*
            send shortest paths to AWS
        */
        if (sendto(serverSocketA, buffer_result, p - buffer_result, 0, (struct sockaddr *)&aws_addr, fromlen) < 0) {
            cout<<"Error occurred when sending shortest paths to AWS."<<endl;
            close(serverSocketA);
            return 0;
        }
	  	cout<<"The Server A has sent shortest paths to AWS."<<endl;
    }

    close(serverSocketA);
    return 0;
}