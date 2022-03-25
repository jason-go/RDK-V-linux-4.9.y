include $(GXSRC_PATH)/scripts/$(ARCH)-$(OS)-compiler.mak

FIRMWARE_SRC='./src'

CFLAGS += -ffunction-sections -fdata-sections

all:
	@sh ../scripts/make.sh $(CROSS_COMPILE)

clean:
	@rm $(FIRMWARE_SRC)/*.[o,c] -f
	@rm $(FIRMWARE_SRC)/libfirmware.a -f
