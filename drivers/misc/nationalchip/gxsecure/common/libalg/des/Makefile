include $(GX_SRC_PATH)/scripts/$(ARCH)-$(OS)-compiler.mk
CC              = $(CROSS_COMPILE)gcc
AR              = $(CROSS_COMPILE)ar

COBJS-y = des_enc.o ecb3_enc.o ede_cbcm_enc.o ncbc_enc.o set_key.o

# The pre-processor and compiler options.   
MY_CFLAGS = -c -O2 \
			-mcpu=ck802 -mlittle-endian -Wpointer-arith -Wundef -Wall -Wundef -Werror \
			-Wstrict-prototypes -pipe -ffunction-sections -fdata-sections -fno-builtin

MY_INCLUDE_DIR = ./

INCLUDE_CMPL_DIR =$(addprefix -I , $(MY_INCLUDE_DIR))

CCFLAGS = $(MY_CFLAGS) $(INCLUDE_CMPL_DIR)

# Rules for generating the executable.   
#-------------------------------------   
lib:libdes.a
	@echo [Create libdes.a]

$(COBJS-y):%.o:%.c
	@echo [$(CC) compiling $@]
	@$(CC) $(CCFLAGS) -c $< -o $@

libdes.a: $(COBJS-y)
	@$(AR) -rcs $@ $^
	@rm -f $(COBJS-y)

clean:   
	@$(RM) $(COBJS-y) libdes.a
	@echo "[clean done]"
  
