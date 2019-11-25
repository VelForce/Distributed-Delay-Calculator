all:
	g++ -std=c++11 -o serverA serverA.cpp
	g++ -std=c++11 -o serverB serverB.cpp
	g++ -std=c++11 -o aws aws.cpp
	g++ -std=c++11 -o client client.cpp

.PHONY: serverA
serverA:
	./serverA

.PHONY: serverB
serverB:
	./serverB

.PHONY: aws
aws:
	./aws