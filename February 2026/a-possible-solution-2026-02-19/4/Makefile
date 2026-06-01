
CC=g++
#CFLAGS=-std=c++11 -lm -I. -I/usr/include/postgresql -lpq 
#CFLAGS=-std=c++11 -I. -I/usr/include/postgresql -lpq -lm 
CFLAGS=-std=c++20 -I.  -lm 

#SUBDIRS=uav montecarlo global input-reader mainfile network output-writer random-engine simulator system monitor1

SUBDIRS := $(shell find . -maxdepth 1 -type d -not -path '.' -printf '%f ')

$(info $$SUBDIRS is [${SUBDIRS}])

#DEPS = $(wildcard *.h)
#objects := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

DEPS = $(wildcard $(IDIR)/*.h)
#OBJ = $(wildcard $(ODIR)/*.o)
SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp, $(ODIR)/%.o, $(SRC))

# 1. Trova i .cpp (che esistono)
# 2. Sostituisce .cpp con .o nella stringa
program_OBJS := $(foreach dir, $(SUBDIRS), $(patsubst %.cpp, %.o, $(wildcard $(dir)/*.cpp)))

$(info $$program_OBJS is [${program_OBJS}])

# Aggiungiamo main1 alla lista all
all: $(SUBDIRS) main1

# Regola per compilare le sottodirectory
$(SUBDIRS):
	$(MAKE) -C $@

# Regola per il link finale
# NOTA: main1 deve dipendere da $(SUBDIRS) per essere sicuro che i .o siano pronti
main1: $(SUBDIRS)
	$(CC) -o main $(program_OBJS) $(CFLAGS)

.PHONY: clean cleanall $(SUBDIRS)

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f main *.o *~

cleanall: clean
	rm -f main *.o *~



