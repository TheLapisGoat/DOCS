all: server client

server: server_TCP.cpp
	g++ -o server server_TCP.cpp -lpthread

client: client_TCP.cpp
	g++ -o client client_TCP.cpp -lpthread

.PHONY: clean
clean: clean_doxygen
	rm -f server client *.out

.PHONY: clean_doxygen
clean_doxygen:
	rm -rf latex

doxygen:
	doxygen Doxyfile
	cd latex && make