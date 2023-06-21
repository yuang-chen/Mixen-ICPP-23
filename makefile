CXX     = g++
CPPFLAGS = -O3  -std=c++11 -fopenmp -mavx -march=native -m64 -ftree-vectorize -D_GLIBCXX_PARALLEL
DEBUG= -g -O0 -Wall  -std=c++11 -fopenmp -mavx -w  -D_GLIBCXX_PARALLEL
LDFLAGS = -fopenmp -lpthread  -lboost_timer -lboost_chrono -lboost_system -lboost_program_options#-fopenmp-simd 
INC:=./include/
APP:=./app

target = pr
#all: $(SOURCES) moca
debug = pr


# export CPATH={boost_dir}/boost-1.75/include:$CPATH
# export LD_LIBRARY_PATH={boost_dir}/boost-1.75/lib:$LD_LIBRARY_PATH
# export LIBRARY_PATH={boost_dir}/boost-1.75/lib:$LIBRARY_PATH

BOOST_INC={boost_dir}/boost-1.75/include
BOOST_LIB={boost_dir}/boost-1.75/lib

CPPFLAGS+=-I$(BOOST_INC)
LDFLAGS+=-L$(BOOST_LIB)
all: $(target)

$(target): %: $(APP)/%.cpp 
	$(CXX) $(CPPFLAGS)  -o $(APP)/$@ $(DEP) -I $(INC) $<  $(LDFLAGS)

test: $(APP)/test.cpp
	$(CXX) $(CPPFLAGS)  -o $(APP)/$@  -I $(INC)  $<   $(LDFLAGS)

debug: $(APP)/$(debug).cpp $(DEP) 
	$(CXX) $(DEBUG)  -o $(APP)/$(debug) $(DEP) -I $(INC) $<  $(LDFLAGS)

clean:
	rm -f $(APP)/$(target)

