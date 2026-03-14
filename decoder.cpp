#include <iostream>

extern "C" {
    // persistent variables to track state across multiple calls
    static int state = 0;         // 0 = search, 1 = decoding
    static int sample_count = 0;  // count samples (0-50)frame
    static int bit_buffer = 0;    // holds the bits as they come in
    static int bits_received = 0; // counts data bits (0-7)
    static int last_val = 1;      // detect the falling edge (1 -> 0)

    /**
     * process_sample
     * input_bit: -1 (silence), 0 (3500Hz), or 1 (4500Hz)
     * returns: ASCII char if a full byte is decoded, otherwise 0
     */
    int process_sample(int input_bit) {
        int result_char = 0;

        // S0: searching for the Start Bit (FALLING EDGE)
        if (state == 0) {
            if (last_val >= 1 && input_bit == 0) {
                state = 1;          // TRIGGER!
                sample_count = 0;
                bits_received = 0;
                bit_buffer = 0;
            }
        } 
        // S1: decoding frame
        else if (state == 1) {
            sample_count++;

            // sample at the center of each bit.
            // start bit is samples 0-4. First data bit center is at 7.
            // subsequent centers: 12, 17, 22, 27, 32, 37, 42.
            if ((sample_count - 8) % 5 == 0 && bits_received < 8 && sample_count >= 8) {
                if (input_bit == 1) {
                    bit_buffer |= (1 <<(7- bits_received)); // build LSB-first
                }
                bits_received++;
            }

            // end of frame (50 samples = 10 bits)
            if (sample_count >= 49) {
                result_char = bit_buffer;
                state = 0; // Go back to hunting
            }
        }

        last_val = input_bit;
        return result_char;
    }
}
