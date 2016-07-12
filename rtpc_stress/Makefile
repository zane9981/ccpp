CXX=gcc

CXXFLAGS = -g

SRCS1=rtpc_stress.c
SRCS2=rtpc_stress2.c

LIBS=-lpthread

INCLUDES=
OBJS=$(SRCS:.c=.o)

EXE1=rtpc_stress
EXE2=rtpc_stress2

all: $(EXE1) $(EXE2)

$(EXE1): $(SRCS1)
	$(CXX) $(CXXFLAGS) -o $(EXE1) $(SRCS1) $(LIBS) $(INCLUDES)

$(EXE2): $(SRCS2)
	$(CXX) $(CXXFLAGS) -o $(EXE2) $(SRCS2) $(LIBS) $(INCLUDES)

clean :
	-rm -rf $(EXE1) $(EXE2) $(OBJS)
