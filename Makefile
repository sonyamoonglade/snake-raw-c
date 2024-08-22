run:
	rm -rf .build && mkdir .build && gcc -o .build/app -Wextra -Iinclude main.c termraw.c -lpthread -lc && .build/app
