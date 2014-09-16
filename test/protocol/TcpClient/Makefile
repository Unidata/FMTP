CC = g++
INCLUDE =
LIB =
ELFFILE = TcpClient

$(ELFFILE): TcpClientTest.cpp TcpClient.cpp TcpClient.h
	$(CC) -o $(ELFFILE) TcpClientTest.cpp TcpClient.cpp -I$(INCLUDE) -l$(LIB)

.PHONY : clean
clean:
	rm $(ELFFILE)
