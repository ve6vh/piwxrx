/* Perform standard 6db/octave de-emphasis */
static int16_t deemph(int16_t input,int32_t *state)
{

int16_t coeff00 = 6878;
int16_t coeff01 = 25889;
int32_t accum; /* 32 bit accumulator */

        accum = input;
        /* YES! The parenthesis REALLY do help on this one! */
        *state = accum + ((*state * coeff01) >> 15);
        accum = (*state * coeff00);
        /* adjust gain so that we have unity @ 1KHz */
        return((accum >> 14) + (accum >> 15));
}