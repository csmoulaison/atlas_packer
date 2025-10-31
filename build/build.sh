mkdir ../bin
cp -r fonts ../bin/

g++ -g -o ../bin/atlas \
	../src/main.cpp \
	-I ../src/ \
	-I ../dependencies/freetype-2.14.1/out/include/freetype2/ \
	-L ../dependencies/freetype-2.14.1/out/lib/ \
	-lfreetype
