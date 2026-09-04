/* Freeverb Algorithm */
#include <stdio.h>
#include <string.h>
#include "reverb.h"

#define undenormalise(sample) if(((*(unsigned int*)&(sample))&0x7f800000)==0) (sample)=0.0f

#define numcombs        8
#define numallpasses    4
static const float muted        = 0;
static const float fixedgain    = 0.015f;
static const float scalewet     = 3;
static const float scaledry     = 2;
static const float scaledamp    = 0.4f;
static const float scaleroom    = 0.28f;
static const float offsetroom   = 0.7f;
static const float initialroom  = 0.5f;
static const float initialdamp  = 0.5f;
#define initialwet (1.0f/scalewet)
static const float initialdry   = 0;
static const float initialwidth = 1;
static const float initialmode  = 0;
static const float freezemode   = 0.5f;
#define stereospread 23

#define combtuningL1   1116
#define combtuningR1   (1116+stereospread)
#define combtuningL2   1188
#define combtuningR2   (1188+stereospread)
#define combtuningL3   1277
#define combtuningR3   (1277+stereospread)
#define combtuningL4   1356
#define combtuningR4   (1356+stereospread)
#define combtuningL5   1422
#define combtuningR5   (1422+stereospread)
#define combtuningL6   1491
#define combtuningR6   (1491+stereospread)
#define combtuningL7   1557
#define combtuningR7   (1557+stereospread)
#define combtuningL8   1617
#define combtuningR8   (1617+stereospread)
#define allpasstuningL1 556
#define allpasstuningR1 (556+stereospread)
#define allpasstuningL2 441
#define allpasstuningR2 (441+stereospread)
#define allpasstuningL3 341
#define allpasstuningR3 (341+stereospread)
#define allpasstuningL4 225
#define allpasstuningR4 (225+stereospread)
typedef struct comb {
    float   feedback;
    float   filterstore;
    float   damp1;
    float   damp2;
    float   *buffer;
    int     bufsize;
    int     bufidx;
} comb;

typedef struct allpass {
    float   feedback;
    float   *buffer;
    int     bufsize;
    int     bufidx;
} allpass;

typedef struct reverb {
    float   gain;
    float   roomsize, roomsize1;
    float   damp, damp1;
    float   wet, wet1, wet2;
    float   dry;
    float   width;
    float   mode;

    comb    combL[numcombs];
    comb    combR[numcombs];

    allpass allpassL[numallpasses];
    allpass allpassR[numallpasses];

    float   bufcombL1[combtuningL1];
    float   bufcombR1[combtuningR1];
    float   bufcombL2[combtuningL2];
    float   bufcombR2[combtuningR2];
    float   bufcombL3[combtuningL3];
    float   bufcombR3[combtuningR3];
    float   bufcombL4[combtuningL4];
    float   bufcombR4[combtuningR4];
    float   bufcombL5[combtuningL5];
    float   bufcombR5[combtuningR5];
    float   bufcombL6[combtuningL6];
    float   bufcombR6[combtuningR6];
    float   bufcombL7[combtuningL7];
    float   bufcombR7[combtuningR7];
    float   bufcombL8[combtuningL8];
    float   bufcombR8[combtuningR8];
    float   bufallpassL1[allpasstuningL1];
    float   bufallpassR1[allpasstuningR1];
    float   bufallpassL2[allpasstuningL2];
    float   bufallpassR2[allpasstuningR2];
    float   bufallpassL3[allpasstuningL3];
    float   bufallpassR3[allpasstuningR3];
    float   bufallpassL4[allpasstuningL4];
    float   bufallpassR4[allpasstuningR4];
} reverb;

static reverb rv;

static void comb_init(comb *cm) {
    cm->filterstore = 0;
    cm->bufidx = 0;
}

static void cmsetbuffer(comb *cm, float *buf, int size) {
    cm->buffer = buf;
    cm->bufsize = size;
}

static void cmmute(comb *cm) {
    int i;
    for (i = 0; i < cm->bufsize; i++)
        cm->buffer[i] = 0;
}

static void cmsetdamp(comb *cm, float val) {
    cm->damp1 = val;
    cm->damp2 = 1 - val;
}

static float cmgetdamp(comb *cm) {
    return cm->damp1;
}

static void cmsetfeedback(comb *cm, float val) {
    cm->feedback = val;
}

static float cmgetfeedback(comb *cm) {
    return cm->feedback;
}

static inline float cmprocess(comb *cm, float input) {
    float output;

    output = cm->buffer[cm->bufidx];
    undenormalise(output);

    cm->filterstore = (output * cm->damp2) + (cm->filterstore * cm->damp1);
    undenormalise(cm->filterstore);

    cm->buffer[cm->bufidx] = input + (cm->filterstore * cm->feedback);

    if (++cm->bufidx >= cm->bufsize) cm->bufidx = 0;

    return output;
}


static void allpass_init(allpass *al) {
    al->bufidx = 0;
}

static void alsetbuffer(allpass *al, float *buf, int size) {
    al->buffer = buf;
    al->bufsize = size;
}

static void almute(allpass *al) {
    int i;
    for (i = 0; i < al->bufsize; i++)
        al->buffer[i] = 0;
}

static void alsetfeedback(allpass *al, float val) {
    al->feedback = val;
}

static float algetfeedback(allpass *al) {
    return al->feedback;
}

static inline float alprocess(allpass *al, float input) {
    float output;
    float bufout;

    bufout = al->buffer[al->bufidx];
    undenormalise(bufout);

    output = -input + bufout;
    al->buffer[al->bufidx] = input + (bufout * al->feedback);

    if (++al->bufidx >= al->bufsize) al->bufidx = 0;

    return output;
}


static void rvupdate(void);
static void rvsetroomsize(float value);
static float rvgetroomsize(void);
static void rvsetdamp(float value);
static float rvgetdamp(void);
static void rvsetwet(float value);
static float rvgetwet(void);
static void rvsetdry(float value);
static float rvgetdry(void);
static void rvsetwidth(float value);
static float rvgetwidth(void);
static void rvsetmode(float value);
static float rvgetmode(void);

static void rvmute(void)
{
    int i;

    if (rvgetmode() >= freezemode)
        return;

    for (i = 0; i < numcombs; i++) {
        cmmute(&rv.combL[i]);
        cmmute(&rv.combR[i]);
    }
    for (i = 0; i < numallpasses; i++) {
        almute(&rv.allpassL[i]);
        almute(&rv.allpassR[i]);
    }
}

static void rvprocessreplace(float *inputL, float *inputR, float *outputL, float *outputR, long numsamples, int skip)
{
    float outL, outR, input;
    int i;

    while (numsamples-- > 0) {
        outL = outR = 0;
        input = (*inputL + *inputR) * rv.gain;

        for (i = 0; i < numcombs; i++) {
            outL += cmprocess(&rv.combL[i], input);
            outR += cmprocess(&rv.combR[i], input);
        }

        for (i = 0; i < numallpasses; i++) {
            outL = alprocess(&rv.allpassL[i], outL);
            outR = alprocess(&rv.allpassR[i], outR);
        }

        *outputL = outL * rv.wet1 + outR * rv.wet2 + *inputL * rv.dry;
        *outputR = outR * rv.wet1 + outL * rv.wet2 + *inputR * rv.dry;

        inputL += skip;
        inputR += skip;
        outputL += skip;
        outputR += skip;
    }
}

static void rvprocessmix(float *inputL, float *inputR, float *outputL, float *outputR, long numsamples, int skip)
{
    float outL, outR, input;
    int i;

    while (numsamples-- > 0) {
        outL = outR = 0;
        input = (*inputL + *inputR) * rv.gain;

        for (i = 0; i < numcombs; i++) {
            outL += cmprocess(&rv.combL[i], input);
            outR += cmprocess(&rv.combR[i], input);
        }

        for (i = 0; i < numallpasses; i++) {
            outL = alprocess(&rv.allpassL[i], outL);
            outR = alprocess(&rv.allpassR[i], outR);
        }
        
        *outputL += outL * rv.wet1 + outR * rv.wet2 + *inputL * rv.dry;
        *outputR += outR * rv.wet1 + outL * rv.wet2 + *inputR * rv.dry;

        inputL += skip;
        inputR += skip;
        outputL += skip;
        outputR += skip;
    }
}

static void rvupdate(void)
{
    int i;

    rv.wet1 = rv.wet * (rv.width / 2 + 0.5f);
    rv.wet2 = rv.wet * ((1 - rv.width) / 2);

    if (rv.mode >= freezemode) {
        rv.roomsize1 = 1;
        rv.damp1 = 0;
        rv.gain = muted;
    } else {
        rv.roomsize1 = rv.roomsize;
        rv.damp1 = rv.damp;
        rv.gain = fixedgain;
    }

    for (i = 0; i < numcombs; i++) {
        cmsetfeedback(&rv.combL[i], rv.roomsize1);
        cmsetfeedback(&rv.combR[i], rv.roomsize1);
    }

    for (i = 0; i < numcombs; i++) {
        cmsetdamp(&rv.combL[i], rv.damp1);
        cmsetdamp(&rv.combR[i], rv.damp1);
    }
}


static void rvsetroomsize(float value)
{
    rv.roomsize = (value * scaleroom) + offsetroom;
    rvupdate();
}

static float rvgetroomsize(void)
{
    return (rv.roomsize - offsetroom) / scaleroom;
}

static void rvsetdamp(float value)
{
    rv.damp = value * scaledamp;
    rvupdate();
}

static float rvgetdamp(void)
{
    return rv.damp / scaledamp;
}

static void rvsetwet(float value)
{
    rv.wet = value * scalewet;
    rvupdate();
}

static float rvgetwet(void)
{
    return rv.wet / scalewet;
}

static void rvsetdry(float value)
{
    rv.dry = value * scaledry;
}

static float rvgetdry(void)
{
    return rv.dry / scaledry;
}

static void rvsetwidth(float value)
{
    rv.width = value;
    rvupdate();
}

static float rvgetwidth(void)
{
    return rv.width;
}

static void rvsetmode(float value)
{
    rv.mode = value;
    rvupdate();
}

static float rvgetmode(void)
{
    if (rv.mode >= freezemode)
        return 1;
    else
        return 0;
}

static void revmodel_init(void)
{
    int i;

    comb_init(&rv.combL[0]); comb_init(&rv.combR[0]);
    comb_init(&rv.combL[1]); comb_init(&rv.combR[1]);
    comb_init(&rv.combL[2]); comb_init(&rv.combR[2]);
    comb_init(&rv.combL[3]); comb_init(&rv.combR[3]);
    comb_init(&rv.combL[4]); comb_init(&rv.combR[4]);
    comb_init(&rv.combL[5]); comb_init(&rv.combR[5]);
    comb_init(&rv.combL[6]); comb_init(&rv.combR[6]);
    comb_init(&rv.combL[7]); comb_init(&rv.combR[7]);

    for (i = 0; i < numallpasses; i++) {
        allpass_init(&rv.allpassL[i]);
        allpass_init(&rv.allpassR[i]);
    }

    cmsetbuffer(&rv.combL[0], rv.bufcombL1, combtuningL1);
    cmsetbuffer(&rv.combR[0], rv.bufcombR1, combtuningR1);
    cmsetbuffer(&rv.combL[1], rv.bufcombL2, combtuningL2);
    cmsetbuffer(&rv.combR[1], rv.bufcombR2, combtuningR2);
    cmsetbuffer(&rv.combL[2], rv.bufcombL3, combtuningL3);
    cmsetbuffer(&rv.combR[2], rv.bufcombR3, combtuningR3);
    cmsetbuffer(&rv.combL[3], rv.bufcombL4, combtuningL4);
    cmsetbuffer(&rv.combR[3], rv.bufcombR4, combtuningR4);
    cmsetbuffer(&rv.combL[4], rv.bufcombL5, combtuningL5);
    cmsetbuffer(&rv.combR[4], rv.bufcombR5, combtuningR5);
    cmsetbuffer(&rv.combL[5], rv.bufcombL6, combtuningL6);
    cmsetbuffer(&rv.combR[5], rv.bufcombR6, combtuningR6);
    cmsetbuffer(&rv.combL[6], rv.bufcombL7, combtuningL7);
    cmsetbuffer(&rv.combR[6], rv.bufcombR7, combtuningR7);
    cmsetbuffer(&rv.combL[7], rv.bufcombL8, combtuningL8);
    cmsetbuffer(&rv.combR[7], rv.bufcombR8, combtuningR8);

    alsetbuffer(&rv.allpassL[0], rv.bufallpassL1, allpasstuningL1);
    alsetbuffer(&rv.allpassR[0], rv.bufallpassR1, allpasstuningR1);
    alsetbuffer(&rv.allpassL[1], rv.bufallpassL2, allpasstuningL2);
    alsetbuffer(&rv.allpassR[1], rv.bufallpassR2, allpasstuningR2);
    alsetbuffer(&rv.allpassL[2], rv.bufallpassL3, allpasstuningL3);
    alsetbuffer(&rv.allpassR[2], rv.bufallpassR3, allpasstuningR3);
    alsetbuffer(&rv.allpassL[3], rv.bufallpassL4, allpasstuningL4);
    alsetbuffer(&rv.allpassR[3], rv.bufallpassR4, allpasstuningR4);

    alsetfeedback(&rv.allpassL[0], 0.5f);
    alsetfeedback(&rv.allpassR[0], 0.5f);
    alsetfeedback(&rv.allpassL[1], 0.5f);
    alsetfeedback(&rv.allpassR[1], 0.5f);
    alsetfeedback(&rv.allpassL[2], 0.5f);
    alsetfeedback(&rv.allpassR[2], 0.5f);
    alsetfeedback(&rv.allpassL[3], 0.5f);
    alsetfeedback(&rv.allpassR[3], 0.5f);

    rvsetwet(initialwet);
    rvsetroomsize(initialroom);
    rvsetdry(initialdry);
    rvsetdamp(initialdamp);
    rvsetwidth(initialwidth);
    rvsetmode(initialmode);

    rvmute();
}


void reverb_init(void)
{
    revmodel_init();
}

void reverb_set_roomsize(float value) { rvsetroomsize(value); }
void reverb_set_damp(float value)     { rvsetdamp(value); }
void reverb_set_wet(float value)      { rvsetwet(value); }
void reverb_set_dry(float value)      { rvsetdry(value); }
void reverb_set_width(float value)    { rvsetwidth(value); }
void reverb_process_replace_stereo(float *interleaved, int nframes, int channels)
{
    if (channels != 2 || nframes <= 0 || interleaved == NULL)
        return;

    rvprocessreplace(interleaved, interleaved + 1,
                      interleaved, interleaved + 1,
                      nframes, channels);
}
