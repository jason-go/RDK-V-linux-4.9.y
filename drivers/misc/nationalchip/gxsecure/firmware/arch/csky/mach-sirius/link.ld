ENTRY(stage1_start);

MEMORY
{
    SRAM : ORIGIN = 0x00090000, LENGTH = 192*1024
}

__user_stack  = 0x000c0000 - 0x1000 -4;
__super_stack = 0x000c0000 - 4;

SECTIONS
{
	.text : {
		_start_text_ = .;
		*(.text*)
		*(.rodata*)
		. = ALIGN(4);
		_end_text = .;
	} > SRAM

	.data : {
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
