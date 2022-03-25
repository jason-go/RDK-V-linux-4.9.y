ENTRY(stage1_start);

MEMORY
{
    DDR  : ORIGIN = 0x00040000, LENGTH = 256*1024
    SRAM : ORIGIN = 0x00080000, LENGTH = 12*1024
    DRAM : ORIGIN = 0x12000000, LENGTH = 16*1024
}

__user_stack  = 0x00084000 - 4;

SECTIONS
{
	.text : {
		_start_text_ = .;
		*(.text*)
		*(.rodata*)
		. = ALIGN(4);
		_end_text = .;
	} > DDR

	.data :
	AT (ADDR(.text) + SIZEOF(.text))
	{
		_start_data = .;
		*(.data*)
		. = ALIGN(4);
		_end_data = .;
	} > SRAM

	.bss : {
		_start_bss_ = .;
		*(.bss*) *(COMMON)
		. = ALIGN(4);
		_end_bss_ = .;
	} > SRAM
}
