run:
	rm -rf .build && mkdir .build && gcc -o .build/app -Wextra -lc -Iinclude main.c termraw.c && .build/app
