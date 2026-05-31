
# Trova tutte le directory (terminano con /)
DIRS_WITH_SLASH := $(wildcard ../*/)

#$(info $$DIRS_WITH_SLASH is [${DIRS_WITH_SLASH}])

# Rimuovi lo slash finale (opzionale, ma pulito)
# La funzione 'patsubst' sostituisce "qualcosa/" con "qualcosa"
RAW_DIRS := $(patsubst %/,%,$(DIRS_WITH_SLASH))

#$(info $$SRAW_DIRS is [${RAW_DIRS}])

# 3. Aggiungi il prefisso -I
INCLUDE_DIRS := $(addprefix -I,$(RAW_DIRS))

CC=g++

#CFLAGS=-std=c++11 -lm -I. -I/usr/include/postgresql -lpq 
#CFLAGS=-std=c++11 -I. -I/usr/include/postgresql -lpq -lm 
CFLAGS=-std=c++17 -I. $(INCLUDE_DIRS) -lm


DEPS = $(wildcard *.h)
objects := $(patsubst %.cpp,%.o,$(wildcard *.cpp))


all:	$(objects)

%.o:	%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

#main: 	$(objects)
#	$(CC) -o main $(objects) $(CFLAGS)


.PHONY: clean cleanall

clean:
	rm -f *.o ; rm -f *~


cleanall:
	rm -f main ; rm -f *.o ; rm -f *~
