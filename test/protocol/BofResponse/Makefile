CC = g++
INCLUDE = ../../../protocol/
LIB = cppunit
ELFFILE = UnitTest

$(ELFFILE): BofResponseTest.cpp BofResponse.cpp BofResponse.h
	$(CC) -o $(ELFFILE) BofResponseTest.cpp BofResponse.cpp -I$(INCLUDE) -l$(LIB)

.PHONY : clean
clean:
	rm $(ELFFILE)
