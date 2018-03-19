WeaveRootDir = $(COMPONENT_PATH)/weave
WeaveOutputDir = $(WeaveRootDir)/output
WeaveTargetLibDir = $(WeaveOutputDir)/xtensa-esp32-elf/lib

COMPONENT_ADD_INCLUDEDIRS = weave/output/include \
							weave/output/include/micro-ecc \
							weave/output/include/mincrypt

COMPONENT_OWNBUILDTARGET = 1
COMPONENT_OWNCLEANTARGET = 1
COMPONENT_ADD_LDFLAGS = -L$(WeaveTargetLibDir) \
					    -lWeave \
					    -lInetLayer \
					    -lmincrypt \
					    -lnlfaultinjection \
					    -lSystemLayer \
					    -l uECC \
					    -lWarm

build :

clean :
