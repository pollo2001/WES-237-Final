#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstdio>

extern "C" {    
    int detect_and_decode_loop(
        int frequency1, 
        int frequency2,
        int sample_rate,
        int input_size,
        int* input_buffer, 
        double* output_buffer1, 
        double* output_buffer2,
        int* start_flag,
        int* end_flag
    ) {
        // ---------- GOERTZEL VARS -----------------//
        double sum = 0;
        double avg = 0;
        
        // FREQ 1
        float k1 = (int)(0.5 + ((input_size * frequency1) / sample_rate));
        float omega1 = (2.0 * M_PI * k1) / input_size;
        float cosine1 = std::cos(omega1);
        float coeff1 = 2.0 * cosine1;
        
        // FREQ 2
        float k2 = (int)(0.5 + ((input_size * frequency2) / sample_rate));
        float omega2 = (2.0 * M_PI * k2) / input_size;
        float cosine2 = std::cos(omega2);
        float coeff2 = 2.0 * cosine2;
        
        // OUTPUT
        int bit_rx = 0;        // bit received from goertzel
        
        // ------------- DECODER VARS --------------- //
        int state = 0;         // 0 = search, 1 = decoding
        int sample_count = 0;  // count samples (0-50)frame
        int bit_buffer = 0;    // holds the bits as they come in
        int bits_received = 0; // counts data bits (0-7)
        int last_val = 1;      // detect the falling edge (1 -> 0)
        int result_char = 0;   // output char
        int val = 0;           // temp
        
        //////////////////////////// MAIN LOOP ////////////////////////////////
        int loop_count = 0;
        printf("listening\n");
        fflush(stdout);
        while(*end_flag) {
            if(*start_flag) {
                // --------------------- GOERTZEL ---------------------//
                sum = 0;
                for (int i = 0; i < input_size; i++) {
                    sum += input_buffer[i];
                    avg = sum/input_size;
                }
                
                double q0 = 0, q1 = 0, q2 = 0;
                double r0 = 0, r1 = 0, r2 = 0;
                double rms = 0;
                
                for (int i = 0; i < input_size; i++) {
                    q0 = coeff1 * q1 - q2 + input_buffer[i*2+1] - avg;
                    q2 = q1;
                    q1 = q0;

                    r0 = coeff2 * r1 - r2 + input_buffer[i*2+1] - avg;
                    r2 = r1;
                    r1 = r0;
                    
                    rms += pow(input_buffer[i*2+1], 2) - avg;
                }
                
                rms = sqrt(rms);
                
                output_buffer1[0]  = (q1 * q1 + q2 * q2 - q1 * q2 * coeff1);
                output_buffer2[0]  = (r1 * r1 + r2 * r2 - r1 * r2 * coeff2);
                *start_flag = 0;
                //printf("%f, %f\n", output_buffer1[0], output_buffer2[0]);
                //fflush(stdout);
                
                //
                if (output_buffer1[0] > pow(10, 17) && output_buffer1[0] > 3*output_buffer2[0]) bit_rx = 0;
                else if (output_buffer2[0] > pow(10, 17) && output_buffer2[0] > 3*output_buffer1[0]) bit_rx = 1;
                else bit_rx = -1;
                
                //if(bit_rx == 0) printf("%d: %d: %f\n", sample_count, bit_rx, output_buffer1[0]);
                //else if(bit_rx == 1) printf("%d: %d: %f\n", sample_count, bit_rx, output_buffer2[0]);
                //else printf("%d: x: %f\n", sample_count, output_buffer1[0]);
                //printf("%d: %d: %.2e, %.2e, %.2e\n", sample_count+1, bit_rx, output_buffer1[0], output_buffer2[0], rms);
                //printf("%f\n", output_buffer1[0]);
                //printf("%f\n", output_buffer2[0]);
                //printf("%d", bit_rx);
                //fflush(stdout);
                
                // ---------------------- DECODE ----------------------------//
                result_char = 0;
                
                val = (bit_rx == -1) ? 1 : bit_rx;

                // S0: searching for the Start Bit (FALLING EDGE)
                if (state == 0) {
                    if (last_val >= 1 && val == 0) {
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
                    if ((sample_count - 8) % 5 == 0 && bits_received < 8 && sample_count >= 8 && sample_count < 48) {
                        if (val == 1) {
                            //printf("bit_buffer: %c\n", bit_buffer);
                            //fflush(stdout);
                            bit_buffer |= (1 <<(7- bits_received)); // build MSB-first
                            //printf("bit_buffer: %c\n", bit_buffer);
                            //fflush(stdout);
                        }
                        bits_received++;
                    }

                    // end of frame (50 samples = 10 bits)
                    if (sample_count >= 48) {
                        result_char = bit_buffer;
                        printf("%c", result_char);
                        fflush(stdout);
                        state = 0; // Go back to hunting
                        sample_count = 0;
                    }
                }

                last_val = val;
            }
            else {
                sleep(0.001);
            }
            
            
        }
        printf("eof");
        fflush(stdout);
        return *end_flag;
    }
}
