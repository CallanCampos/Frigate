// Two writable statics compiled as a file of an overlay (port/cmake/overlay_sections.cmake gives this file the forced
// include of PORT_OVSEC_PROBE_OVERLAY, the title's overlay 162), so that integrate_overlay_sections can check without a
// ROM that an overlay's sections hold its files' statics on this host's object format (Mach-O, ELF or COFF;
// port/hal/fs/ovsec.c), and that loading the overlay sets them back as the DS does: one zeroed, one set.
unsigned char port_test_ovsec_zeroed[16];
unsigned char port_test_ovsec_set[16] = { 1, 2, 3 };
