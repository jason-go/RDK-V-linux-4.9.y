include $(GX_SRC_PATH)/scripts/$(ARCH)-$(OS)-compiler.mk
CC              = $(CROSS_COMPILE)gcc
AR              = $(CROSS_COMPILE)ar

COBJS-y = aes_misc.o aes_ecb.o aes_cfb.o aes_ofb.o aes_ctr.o aes_ige.o aes_wrap.o \
	aes_core.o aes_cbc.o cbc128.o cfb128.o ofb128.o

# The pre-processor and compiler options.   
MY_CFLAGS = -c -O2 \
			-mcpu=ck802 -mlittle-endian -Wpointer-arith -Wundef -Wall -Wundef -Werror \
			-Wstrict-prototypes -pipe -ffunction-sections -fdata-sections -fno-builtin

MY_INCLUDE_DIR = ./include ./include/openssl/

INCLUDE_CMPL_DIR =$(addprefix -I , $(MY_INCLUDE_DIR))

CCFLAGS = $(MY_CFLAGS) $(INCLUDE_CMPL_DIR)

# Rules for generating the executable.   
#-------------------------------------   
lib:libaes.a
	@echo [Create libaes.a]

$(COBJS-y):%.o:%.c
	@echo [$(CC) compiling $@]
	@$(CC) $(CCFLAGS) -c $< -o $@

libaes.a: $(COBJS-y)
	@$(AR) -rcs $@ $^
	@rm -f $(COBJS-y)

clean:   
	@$(RM) $(COBJS-y) libaes.a
	@echo "[clean done]"
  
