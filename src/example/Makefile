all: vm cmp intc int dis
vm:
	clang++ -O3 -std=c++11 -fshort-enums nlsvm.cpp -o nlsvm  -Wall -ffast-math -fstrict-aliasing -fkeep-inline-functions -fno-stack-protector
cmp:
	clang++ -O3 -std=c++11 -fshort-enums nlsc.cpp -o nlsc  -Wall -ffast-math -fstrict-aliasing -fkeep-inline-functions -fno-stack-protector
intc:
	clang++ -O3 -std=c++11 -fshort-enums nlsi.cpp -o nlsi  -Wall -ffast-math -fstrict-aliasing -fkeep-inline-functions -fno-stack-protector
int:
	clang++ -O3 -std=c++11 -fshort-enums inls.cpp -o inls  -Wall -ffast-math -fstrict-aliasing -fkeep-inline-functions -fno-stack-protector
dis:
	clang++ -O3 -std=c++11 -fshort-enums nlsd.cpp -o nlsd   -Wall -ffast-math -fstrict-aliasing -fkeep-inline-functions -fno-stack-protector
lfm:
	clang++ -O3 -std=c++11 -fshort-enums lfm.cpp -o lfm   -Wall -ffast-math -fstrict-aliasing -fkeep-inline-functions -fno-stack-protector