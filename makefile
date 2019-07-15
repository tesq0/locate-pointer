CC=gcc
CFLAGS=`pkg-config --cflags cairo x11`
LDFLAGS=`pkg-config --libs cairo x11`
LIBS=-lm
FILES=`find src/*`

animatedpointer:
	$(CC) -o animated-pointer $(CFLAGS) $(FILES) $(LDFLAGS) $(LIBS) 
