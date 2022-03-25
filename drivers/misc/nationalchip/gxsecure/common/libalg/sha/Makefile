include $(GX_SRC_PATH)/scripts/$(ARCH)-$(OS)-compiler.mk
CC              = $(CROSS_COMPILE)gcc
AR              = $(CROSS_COMPILE)ar

COBJS-y = sha_dgst.o sha1dgst.o sha_one.o sha1_one.o sha256.o sha512.o mem_clr.o sha_api.o

# The pre-processor and compiler options.   
MY_CFLAGS = -c -O2 \
			-mcpu=ck802 -mbig-endian -Wpointer-arith -Wundef -Wall -Wundef -Werror \
			-Wstrict-prototypes -pipe -ffunction-sections -fdata-sections -fno-builtin

MY_INCLUDE_DIR = ./include ./include/openssl

INCLUDE_CMPL_DIR =$(addprefix -I , $(MY_INCLUDE_DIR))

CCFLAGS = $(MY_CFLAGS) $(INCLUDE_CMPL_DIR)

# Rules for generating the executable.   
#-------------------------------------   
lib:libsha.a
	@echo [Create libsha.a]

$(COBJS-y):%.o:%.c
	@echo [$(CC) compiling $@]
	@$(CC) $(CCFLAGS) -c $< -o $@

libsha.a: $(COBJS-y)
	@$(AR) -rcs $@ $^
	@rm -f $(COBJS-y)

clean:   
	@$(RM) $(COBJS-y) libsha.a
	@echo "[clean done]"
  
