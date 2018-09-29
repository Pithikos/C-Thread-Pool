CXXFLAGS = -DTHPOOL_DEBUG -pthread

./bin/example : ./obj/example.o ./obj/thpool.o
	gcc -o $@ $^ $(CXXFLAGS) $(LIBS)

./obj/thpool.o : thpool.c
	gcc -c -o $@ $< $(CXXFLAGS)

./obj/example.o : example.c
	gcc -c -o $@ $< $(CXXFLAGS)

.PHONY : clean

clean:
	rm -f ./bin/* ./obj/*.o
