c_compiler                    = gcc
c_compiler_flags              = -W -Wall
c_compiler_flags_optimization = -Os

rm = rm -f

judge_driver_objects = $(patsubst %.c,%.o,$(wildcard *.c))
judge_driver_headers = $(wildcard *.h)

all: judge-driver

judge-driver: $(judge_driver_headers) $(judge_driver_objects)
	$(c_compiler) $(c_compiler_flags) -o $@ $(judge_driver_objects)

%.o: %.c
	$(c_compiler) $(c_compiler_flags) $(c_compiler_flags_optimization) -c $^

.PHONY: clean
clean:
	-$(rm) judge-driver $(call brace,.o,$(judge_driver_objects)).o

# if $(objects) = 'X.o Y.o Z.o',     $(call brace,.o,$(objects))   = '{X,Y,Z}'
# if $(sources) = 'src/a.c src/b.c', $(call brace,src/,$(sources)) = '{a.c,b.c}'
brace = {$(subst $(space),$(comma),$(subst $(1),,$(2)))}

space = $(empty) $(empty)
comma = ,
