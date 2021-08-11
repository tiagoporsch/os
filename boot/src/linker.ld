ENTRY(boot)
SECTIONS {
	. = 0x7C00;
	.text : {
		*(.boot)
		*(.text)
	}
	/DISCARD/ : {
		*(.eh_frame)
		*(.comment)
	}
	_boot_sectors = (. - 0x7C00) / 512;
}
