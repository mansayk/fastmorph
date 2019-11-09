/*
 * Cryptography related functions
 */


/*
 * Decrypt data
 */
void decrypt(char *input, char *output, size_t output_buffer_size)
{
	unsigned char *base64;
	int len = strlen(input);
	size_t xorlen;
	if(!strlen(KEY)) {
		output = strncpy(output, input, output_buffer_size - 1);
		output[output_buffer_size - 1] = '\0';
		return;
	}

	base64 = b64_decode_ex(input, len, &xorlen);
	for(size_t i = 0; i < xorlen; i++) {
		output[i] = base64[i] ^ KEY[i % (sizeof(KEY)/sizeof(char))];
	}
}
