CXX = g++
CXXFLAGS = -std=c++17 -O2
LDFLAGS = -lpqxx -lpq
TARGET = main.exe
SOURCES = main.cpp database.cpp account.cpp transaction.cpp ui.cpp utils.cpp
HEADERS = database.h account.h transaction.h ui.h utils.h

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: clean