LD_FLAGS := -lboost_system -lboost_thread -lboost_filesystem -O2 -L/usr/X11R6/lib -lm -lX11 -lpthread
CC_FLAGS := -std=c++11

INCLUDE_DIRS := $(shell echo "-I"`find Hoi4ToDefcon/ -type d | tr "\\n" ":" | sed 's/:/ -I/g'`.)

SRC_FILES := $(shell find Hoi4ToDefcon/*.*)

CPP_FILES := $(shell find Hoi4ToDefcon/ -name '*.cpp')
OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))
DEP_FILES := $(addprefix depends/,$(notdir $(CPP_FILES:.cpp=.d)))

VPATH := $(shell find Hoi4ToDefcon/ -type d | tr "\\n" ":" )

# dependency smartness from http://scottmcpeak.com/autodepend/autodepend.html

all: exe

exe: Release/Hoi4ToDefconConverter

-include $(DEP_FILES)

Release/Hoi4ToDefconConverter: $(OBJ_FILES)
	g++ -o $@ $^ $(LD_FLAGS)

obj/%.o: %.cpp
	@mkdir -p obj
	@mkdir -p depends
	g++ $(CC_FLAGS) $(INCLUDE_DIRS) -c -o $@ $<
	@g++ -MM $(CC_FLAGS) $(INCLUDE_DIRS) -c $< > depends/$*.d
	@mv -f depends/$*.d depends/$*.d.tmp
	@sed -e 's|.*:|$@:|' < depends/$*.d.tmp > depends/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < depends/$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> depends/$*.d
	@rm -f depends/$*.d.tmp
	
clean:
	@rm -rf obj
	@rm -rf depends
	@rm -f Release/Hoi4ToDefconConverter

run: all
	./Release/Hoi4ToDefconConverter
