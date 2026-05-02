#include <DRIVERS/AC97/AC97.h>
#include <MEMORY/HEAP/KHEAP.h>
#include <RTOSKRNL_INTERNAL.h>
#include <STD/ASM.h>
#include <STD/MEM.h>
#include <CPU/PIC/PIC.h>
#include <CPU/ISR/ISR.h>
#include <DRIVERS/PCI/PCI.h>

// --------------------------------------------------
// AC97 REGISTERS
// --------------------------------------------------

static U32 AC97_NAM_BASE = 0;
static U32 AC97_NABM_BASE = 0;
static U8  AC97_IRQ = 0;
static PCI_DEVICE_ENTRY *ac97_dev = NULLPTR;

#define NAM_RESET         0x00
#define NAM_MASTER_VOLUME 0x02
#define NAM_PCM_OUT_VOLUME 0x18
#define NAM_FRONT_DAC_RATE 0x2C

#define NABM_PO_BDBAR 0x10
#define NABM_PO_CIV   0x14
#define NABM_PO_LVI   0x15
#define NABM_PO_SR    0x16
#define NABM_PO_CR    0x1B

#define PO_SR_BCIS   (1<<3)
#define PO_SR_LVBCI  (1<<2)
#define PO_SR_MASK   (PO_SR_BCIS|PO_SR_LVBCI)

#define PO_CR_RUN   (1<<0)
#define PO_CR_LVBIE (1<<3)
#define PO_CR_IOCE  (1<<4)

// --------------------------------------------------

typedef struct {
    U32 addr;
    U16 len;
    U16 flags;
} ATTRIB_PACKED AC97_BDL_ENTRY;

#define AC97_BDL_ENTRIES 32
#define AC97_BUF_SIZE    (4096)
#define AC97_ALIGN       0x100
#define AC97_BDL_IOC     (1<<15)

// --------------------------------------------------
// MIXER
// --------------------------------------------------

#define MAX_VOICES 8

typedef struct {
    U32 phase;      // 16.16 fixed
    U32 step;       // phase increment
    U32 duration_ms;
    U16 amp;
    BOOLEAN active;
} AC97_VOICE;

static AC97_VOICE voices[MAX_VOICES];
static U32 sample_rate = 48000; // DAC output rate (hardware fixed)

// 8-bit unsigned mono @ 22050 Hz stream
// Resampled to 48000 Hz via 16.16 fixed-point stepping (step = 22050*65536/48000 = 30109).
// Split into U32 integer index + U16 fraction to avoid needing a U64.
#define PCM8_STEP  30109U
static const U8*  pcm8_data   = NULLPTR;
static U32        pcm8_frames = 0;
static U32        pcm8_idx    = 0; // integer sample index into pcm8_data
static U16        pcm8_frac   = 0; // fractional accumulator (0..65535)

// 16-bit signed stereo @ 48000 Hz stream.
// PCM16_STEP = 65536 (1:1 with 48 kHz AC97 output) means the fractional part
// is always 0, so we track a plain U32 frame counter — no fixed-point needed.
static const U16* pcm16_data   = NULLPTR;
static U32        pcm16_frames = 0; // total frame count  (1 frame = L + R samples)
static U32        pcm16_frame  = 0; // current frame index

static BOOL       g_ac97_paused = FALSE;

// --------------------------------------------------
// VISUALIZER
// --------------------------------------------------
// 8 time-slice energy bands updated each DMA buffer fill.
// Each band holds a smoothed peak amplitude (0..32767).
#define VIZ_BANDS 8
static U32 g_viz_bands[VIZ_BANDS];

static AC97_BDL_ENTRY *Ac97Bdl;
static U8 *Ac97Bufs[AC97_BDL_ENTRIES];

// --------------------------------------------------
// IO
// --------------------------------------------------

static inline VOID NamWrite(U16 r,U16 v){_outw(AC97_NAM_BASE+r,v);}
static inline U8 NabmRead8(U16 r){return _inb(AC97_NABM_BASE+r);}
static inline U16 NabmRead16(U16 r){return _inw(AC97_NABM_BASE+r);}
static inline VOID NabmWrite8(U16 r,U8 v){_outb(AC97_NABM_BASE+r,v);}
static inline VOID NabmWrite16(U16 r,U16 v){_outw(AC97_NABM_BASE+r,v);}
static inline VOID NabmWrite32(U16 r,U32 v){_outl(AC97_NABM_BASE+r,v);}

// --------------------------------------------------
// MIXER CORE (FAST)
// --------------------------------------------------

static VOID AC97_MIX_SAMPLES(U16* buffer, U32 samples)
{
    // samples = number of U16 values; stereo so samples/2 = output frames
    U32 buffer_ms = (samples * 500) / sample_rate;

    for(U32 i = 0; i < samples; i += 2)
    {
        I32 L = 0, R = 0;

        // --- 8-bit mono 22050 Hz stream (resampled to 48000 Hz) ---
        if(!g_ac97_paused && pcm8_data && pcm8_frames > 0)
        {
            if(pcm8_idx >= pcm8_frames)
            {
                pcm8_data   = NULLPTR;
                pcm8_frames = 0;
                pcm8_idx    = 0;
                pcm8_frac   = 0;
            }
            else
            {
                // U8 unsigned -> I16 signed: centre at 0, expand to 16-bit range
                I32 val = ((I32)pcm8_data[pcm8_idx] - 128) << 8;
                L += val;
                R += val; // mono: duplicate to both channels
                // Advance fixed-point accumulator; carry integer part into idx
                U32 next_frac = (U32)pcm8_frac + PCM8_STEP;
                pcm8_idx  += next_frac >> 16;
                pcm8_frac  = (U16)(next_frac & 0xFFFF);
            }
        }

        // --- 16-bit stereo 48000 Hz stream (1:1 with AC97 output rate) ---
        if(!g_ac97_paused && pcm16_data && pcm16_frames > 0)
        {
            if(pcm16_frame >= pcm16_frames)
            {
                pcm16_data   = NULLPTR;
                pcm16_frames = 0;
                pcm16_frame  = 0;
            }
            else
            {
                L += (I16)pcm16_data[pcm16_frame * 2];
                R += (I16)pcm16_data[pcm16_frame * 2 + 1];
                pcm16_frame++;
            }
        }

        // --- Synthesizer voices (square wave tones) ---
        for(U32 v = 0; v < MAX_VOICES; v++)
        {
            if(!voices[v].active) continue;

            voices[v].phase += voices[v].step;

            I16 val = (voices[v].phase & 0x8000) ? voices[v].amp : -voices[v].amp;

            // Simple linear decay in the last 10 ms
            if (voices[v].duration_ms < 10 && voices[v].duration_ms > 0)
                val = (val * (I32)voices[v].duration_ms) / 10;

            L += val;
            R += val;
        }

        if(L > 32767) L = 32767; if(L < -32768) L = -32768;
        if(R > 32767) R = 32767; if(R < -32768) R = -32768;

        buffer[i]     = (U16)(I16)L;
        buffer[i + 1] = (U16)(I16)R;

        // --- Visualizer: accumulate peak per band (time-slice) ---
        {
            U32 slice_frames = (samples / 2) / VIZ_BANDS; // stereo frames per band slice
            if (slice_frames == 0) slice_frames = 1;
            U32 frame_idx = i / 2;                         // current stereo frame index
            U32 band = frame_idx / slice_frames;
            if (band >= VIZ_BANDS) band = VIZ_BANDS - 1;
            U32 peak = (U32)(L < 0 ? -L : L);
            U32 rabs = (U32)(R < 0 ? -R : R);
            if (rabs > peak) peak = rabs;
            // Exponential smoothing: blend toward new peak
            g_viz_bands[band] = (g_viz_bands[band] * 7 + peak) >> 3;
        }
    }

    // Update voice durations
    for(U32 v = 0; v < MAX_VOICES; v++) {
        if(voices[v].active && voices[v].duration_ms != 0xFFFFFFFF) {
            if(voices[v].duration_ms <= buffer_ms) {
                voices[v].active = FALSE;
                voices[v].duration_ms = 0;
            } else {
                voices[v].duration_ms -= buffer_ms;
            }
        }
    }
}

// --------------------------------------------------
// LOW LATENCY PIPELINE UPDATE
// --------------------------------------------------

static VOID AC97_REFRESH_PIPELINE(void)
{
    U8 civ = NabmRead8(NABM_PO_CIV);
    
    // Mix the NEXT 2 buffers in the queue so there is no silence gap
    U8 b1 = (civ + 1) % AC97_BDL_ENTRIES;
    U8 b2 = (civ + 2) % AC97_BDL_ENTRIES;

    AC97_MIX_SAMPLES((U16*)Ac97Bufs[b1], AC97_BUF_SIZE/2);
    AC97_MIX_SAMPLES((U16*)Ac97Bufs[b2], AC97_BUF_SIZE/2);
    
    // Push LVI ahead to cover both mixed buffers
    NabmWrite8(NABM_PO_LVI, b2);

    // If the DMA halted because it played silence and hit LVI, wake it up!
    U8 cr = NabmRead8(NABM_PO_CR);
    if (!(cr & PO_CR_RUN)) {
        NabmWrite8(NABM_PO_CR, cr | PO_CR_RUN);
    }
}

// --------------------------------------------------
// IRQ
// --------------------------------------------------

VOID AC97_HANDLER(U32 vector,U32 err)
{
    (void)err;

    U16 sr=NabmRead16(NABM_PO_SR);

    if(sr & PO_SR_MASK)
    {
        U8 civ=NabmRead8(NABM_PO_CIV);

        // FIX: We must mix into the FUTURE buffer at the front of the queue,
        // not the finished buffer in the past!
        U8 tail = (civ + 2) % AC97_BDL_ENTRIES;

        AC97_MIX_SAMPLES((U16*)Ac97Bufs[tail], AC97_BUF_SIZE/2);

        // Advance the LVI safely
        NabmWrite8(NABM_PO_LVI, tail);
        NabmWrite16(NABM_PO_SR, PO_SR_MASK);
    }

    pic_send_eoi((U8)(vector-PIC_REMAP_OFFSET));
}
// --------------------------------------------------
// INIT
// --------------------------------------------------

BOOLEAN AC97_INIT(void)
{
    U32 count=PCI_GET_DEVICE_COUNT();

    for(U32 i=0;i<count;i++)
    {
        PCI_DEVICE_ENTRY* dev=PCI_GET_DEVICE(i);

        if(dev &&
           dev->header.class_code==0x04 &&
           dev->header.subclass==0x01)
        {
            PCI_ENABLE_BUS_MASTERING(dev->bus,dev->slot,dev->func);

            AC97_NAM_BASE  = dev->header.bar0 & ~3;
            AC97_NABM_BASE = dev->header.bar1 & ~3;
            AC97_IRQ = PCI_READ8(dev->bus,dev->slot,dev->func,0x3C);

            ac97_dev=dev;
            break;
        }
    }

    if(!ac97_dev) return FALSE;

    NamWrite(NAM_RESET,0xFFFF);
    for(volatile int i=0;i<100000;i++);

    NamWrite(NAM_MASTER_VOLUME,0);
    NamWrite(NAM_PCM_OUT_VOLUME,0);
    NamWrite(NAM_FRONT_DAC_RATE,48000);

    Ac97Bdl=KMALLOC_ALIGN(sizeof(AC97_BDL_ENTRY)*AC97_BDL_ENTRIES, AC97_ALIGN);

    for(U32 i=0;i<AC97_BDL_ENTRIES;i++)
    {
        Ac97Bufs[i]=KMALLOC_ALIGN(AC97_BUF_SIZE,AC97_ALIGN);

        AC97_MIX_SAMPLES((U16*)Ac97Bufs[i], AC97_BUF_SIZE/2);

        Ac97Bdl[i].addr=(U32)Ac97Bufs[i];
        Ac97Bdl[i].len =AC97_BUF_SIZE/2;
        Ac97Bdl[i].flags=AC97_BDL_IOC;
    }

    NabmWrite32(NABM_PO_BDBAR,(U32)Ac97Bdl);
    NabmWrite8(NABM_PO_CIV,0);
    NabmWrite8(NABM_PO_LVI,2); // Start safely ahead

    ISR_REGISTER_HANDLER(PIC_REMAP_OFFSET+AC97_IRQ, AC97_HANDLER);
    PIC_Unmask(AC97_IRQ);

    NabmWrite8(NABM_PO_CR, PO_CR_RUN|PO_CR_IOCE|PO_CR_LVBIE);

    return TRUE;
}

// --------------------------------------------------
// API
// --------------------------------------------------

BOOLEAN AC97_TONE(U32 freq,U32 duration,U32 rate,U16 amp)
{
    (void)rate;

    for(U32 i=0;i<MAX_VOICES;i++)
    {
        if(!voices[i].active)
        {
            voices[i].phase=0;
            // 16.16 Fixed Point step calculation
            voices[i].step=(freq<<16)/sample_rate;
            voices[i].amp=(amp?amp:3000);
            voices[i].duration_ms=duration?duration:0xFFFFFFFF;
            voices[i].active=TRUE;

            AC97_REFRESH_PIPELINE();
            return TRUE;
        }
    }
    return FALSE;
}

BOOLEAN AC97_STATUS(void){return ac97_dev!=NULLPTR;}

/* Returns TRUE while PCM data is still being mixed by the IRQ handler */
BOOLEAN AC97_IS_PLAYING(void){return pcm16_frames > 0 || pcm8_frames > 0;}

VOID AC97_PAUSE(BOOL pause){g_ac97_paused = pause;}
BOOL AC97_IS_PAUSED(void){return g_ac97_paused;}

U32 AC97_GET_FRAME_POS(void){return pcm16_frame;}
U32 AC97_GET_8BIT_FRAME_POS(void){return pcm8_idx;}

// Play 8-bit unsigned mono PCM at 22050 Hz.
// Buffer must remain valid for the duration of playback (streamed, not copied).
BOOLEAN AC97_PLAY8(const U8* pcm, U32 frames)
{
    if(!pcm || !frames) return FALSE;
    pcm8_data   = pcm;
    pcm8_frames = frames;
    pcm8_idx    = 0;
    pcm8_frac   = 0;
    AC97_REFRESH_PIPELINE();
    return TRUE;
}

BOOLEAN AC97_PLAY16(const U16* pcm, U32 frames)
{
    if(!pcm || !frames) return FALSE;
    pcm16_data   = pcm;
    pcm16_frames = frames;
    pcm16_frame  = 0;
    AC97_REFRESH_PIPELINE();
    return TRUE;
}

BOOLEAN AC97_GET_VIZ(U32 *bands, U32 n)
{
    if (!bands || n == 0) return FALSE;
    if (n > VIZ_BANDS) n = VIZ_BANDS;
    for (U32 i = 0; i < n; i++)
        bands[i] = g_viz_bands[i];
    return TRUE;
}

VOID AC97_STOP(void)
{
    pcm8_data    = NULLPTR; pcm8_frames  = 0; pcm8_idx  = 0; pcm8_frac = 0;
    pcm16_data   = NULLPTR; pcm16_frames = 0; pcm16_frame = 0;
    g_ac97_paused = FALSE;
    for(U32 i=0;i<MAX_VOICES;i++)
        voices[i].active=FALSE;
}

