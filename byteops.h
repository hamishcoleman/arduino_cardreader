
/* Convert from a memory buffer to an integer */
uint32_t buf_be2hl(uint8_t *buf);
uint32_t buf_le2hl(uint8_t *buf);

/* Dump a memory buffer to serial */
void hexdump(uint8_t *buf, uint8_t size);

/* Print an int with a zeropadded prefix */
void serial_intzeropad(uint32_t i, uint8_t zeropad);
