# Base Makefile for ULTIMA 2.0 - Phase 1
# Prepared for Phase C Integration

CXX = g++
CXXFLAGS = -Wall -g -std=c++11
TARGET = ultima_os

# Object files needed for the final compilation
OBJS = Ultima.o Sched.o Sema.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Zander's main driver
Ultima.o: Ultima.cpp Sched.h Sema.h
	$(CXX) $(CXXFLAGS) -c Ultima.cpp

# Stewart's Scheduler module
Sched.o: Sched.cpp Sched.h
	$(CXX) $(CXXFLAGS) -c Sched.cpp

# Nicholas's Semaphore module
Sema.o: Sema.cpp Sema.h Sched.h
	$(CXX) $(CXXFLAGS) -c Sema.cpp

# Cleanup rule
clean:
	rm -f *.o $(TARGET)