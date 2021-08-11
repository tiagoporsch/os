ENTRY(kernel_main)
SECTIONS {
	. = 0xFFFFFF8000000000;
	/DISCARD/ : {
		*(.eh_frame)
		*(.comment)
	}
}
