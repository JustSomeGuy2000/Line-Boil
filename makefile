# deployment files
SOURCES=main.cpp entities.cpp screens.cpp game.cpp ui.cpp items.cpp
HEADERS=entities.h screens.h game.h ui.h items.h
# name of final .exe
EXE=lineboil
#testing files
CONCAT=concatenated.cpp
CONCAT_EXE=lineboil_concat
#version of C++ standard to use
STD=c++26

run:
	make ${EXE}
	./${EXE}

debug:
	make ${EXE}
	gdb ./${EXE}
# echo run | gdb ./${EXE} to automatically run

# this is a rule. It follows the syntax targets: dependencies
# the first rule is run using just `make`. Other rules must be specified with `make <rule name>`.
# targets are the name(s) of the file(s).
# dependencies are file names
# if any of the targets don't exist or any of the dependencies are newer any of the targets, all commands in the block below will be executed.
# otherwise nothing happens.
${EXE}: ${SOURCES} ${HEADERS}
	clang++ -g ${SOURCES} -o ${EXE} -l Splashkit -Wall -Wextra -std=${STD}

tests: ${TESTS} ${TEST_HEADERS}
	clang++ -g ${TESTS} -o test_${EXE} -l Splashkit -Wall -Wextra -std=${STD}
	./test_${EXE}

make_concat:
	clang++ -g ${CONCAT} -o ${CONCAT_EXE} -l Splashkit -Wall -Wextra -std=${STD}

concat:
	make make_concat
	./${CONCAT_EXE}
