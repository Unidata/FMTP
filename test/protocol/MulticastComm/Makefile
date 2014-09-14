CC = g++
INCLUDE = ../../../protocol/
#LIB =
ELFFILE = MulticastCommTest

$(ELFFILE): MulticastCommTest.cpp MulticastComm.cpp InetComm.cpp MulticastComm.h
	$(CC) -o $(ELFFILE) MulticastCommTest.cpp MulticastComm.cpp InetComm.cpp -I$(INCLUDE)
	#$(CC) -o $(ELFFILE) MulticastCommTest.cpp MulticastComm.cpp -I$(INCLUDE) #-l$(LIB)

.PHONY : clean
clean:
	rm $(ELFFILE)
