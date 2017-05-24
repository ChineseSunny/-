sr=$(wildcard ./src/*.c)
obj=$(patsubst ./src/%.c,./obj/%.o,$(src))

#app:$(obj)
	gcc $^ -o $@

#$(obj):($sr)
	gcc -c $< -o $@
