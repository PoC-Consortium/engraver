# http://stackoverflow.com/questions/714100/os-detecting-makefile
# http://blogs.msdn.com/b/david.wang/archive/2006/03/26/howto-detect-process-bitness.aspx

ifeq ($(OS),Windows_NT)
	OSFLAGS += -D WIN32
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		OS=Windows
		OSFLAGS += -D AMD64
		OS_64BIT=1
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		OSFLAGS += -D IA32
		OS_32BIT=1
	endif
else
	UNAME_S := $(shell uname -s)

	ARCH=
	ifeq ($(UNAME_S),Linux)
		OS=Linux
		OSFLAGS += -D LINUX
		ARCH=$(shell uname -p)
	endif
	ifeq ($(UNAME_S),Darwin)
		OS=Mac
		OSFLAGS += -D OSX
		ARCH=$(shell uname -m)
	endif

	ifeq ($(ARCH),x86_64)
		OSFLAGS += -D AMD64
		OS_64BIT=1
	endif
	ifneq ($(filter %86,$(ARCH)),)
		OSFLAGS += -D IA32
		OS_32BIT=1
	endif
	ifneq ($(filter arm%,$(ARCH)),)
		OSFLAGS += -D ARM
	endif
endif
