LIB_SRC_FILES := src/persistent_hashmap.cpp src/lru_cache.cpp src/storage_engine.cpp
LIB_OBJ_FILES := $(patsubst src/%.cpp,obj/%.o,$(LIB_SRC_FILES))

LRU_CACHE_SIZE := 64
GC_INTERVAL := 30

all: REPL

.PHONY: test clean repl clean_data

repl: REPL
	./REPL

obj/%.o: src/%.cpp
	mkdir -p obj
	g++ -O3 -D LRU_CACHE_SIZE=$(LRU_CACHE_SIZE) -D GC_INTERVAL=$(GC_INTERVAL) -c -o $@ $< -I include

libstorageengine.a: $(LIB_OBJ_FILES)
	ar rcs libstorageengine.a $(LIB_OBJ_FILES)

REPL: src/REPL.cpp libstorageengine.a
	g++ -O3 -o REPL src/REPL.cpp -I include -L . -lstorageengine

# Build and run tests
test: obj/test_storage_engine.o libstorageengine.a
	g++ -o runTests obj/test_storage_engine.o -I include -L . -lstorageengine -lgtest -lgtest_main -lpthread
	./runTests

obj/%.o: tests/%.cpp
	mkdir -p obj
	g++ -O3 -D LRU_CACHE_SIZE=$(LRU_CACHE_SIZE) -D GC_INTERVAL=$(GC_INTERVAL) -c -o $@ $< -I include

clean: clean_data clean_doxygen
	rm -rf obj libstorageengine.a REPL runTests

clean_data:	
	rm -rf data test_data

clean_doxygen:
	rm -rf latex

doxygen:
	doxygen Doxyfile
	cd latex && make