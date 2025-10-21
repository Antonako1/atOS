// AC'97 driver skeleton
// Initializes Intel ICH-compatible AC'97 controller: discover, reset codec, power-up, and prepare DMA.

#include <DRIVERS/AC97/AC97.h>
#include <MEMORY/HEAP/KHEAP.h>
#include <RTOSKRNL_INTERNAL.h>
#include <STD/ASM.h>
#include <STD/MEM.h>
#include <STD/STRING.h>
#include <CPU/PIC/PIC.h>
#include <CPU/ISR/ISR.h>
#include <DRIVERS/PCI/PCI.h>
#include <DEBUG/KDEBUG.h>
#include <CPU/PIT/PIT.h>

// AC'97 PCI device discovery and minimal init skeleton

// NAM (Mixer) and NABM (Bus Master) bases
static U32 AC97_NAM_BASE = 0;  // BAR0 (I/O ports)
static U32 AC97_NABM_BASE = 0; // BAR1 (I/O ports)
static U8 AC97_IRQ = 0;
static PCI_DEVICE_ENTRY *ac97_dev = NULLPTR;

// NAM (Mixer) register offsets (16-bit registers)
#define NAM_RESET               0x00
#define NAM_MASTER_VOLUME       0x02
#define NAM_PCM_OUT_VOLUME      0x18
#define NAM_POWERDOWN_CTRL_STAT 0x26
#define NAM_EXT_AUDIO_ID        0x28
#define NAM_EXT_AUDIO_STAT_CTRL 0x2A
#define NAM_FRONT_DAC_RATE      0x2C

// NABM (Bus Master) register offsets for PCM Out channel
#define NABM_PO_BDBAR           0x10
#define NABM_PO_CIV             0x14
#define NABM_PO_LVI             0x15
#define NABM_PO_SR              0x16
#define NABM_PO_PICB            0x18
#define NABM_PO_PIV             0x1A
#define NABM_PO_CR              0x1B

// NABM Global registers (device-specific; common ICH offsets)
#define NABM_GLOBAL_CONTROL     0x2C
#define NABM_GLOBAL_STATUS      0x30

// PO_SR bits (status) — subset used; exact mapping to be verified against hardware
#define PO_SR_DCH               (1 << 0)  // DMA Controller Halted
#define PO_SR_CELV              (1 << 1)  // Current Equals Last Valid
#define PO_SR_LVBCI             (1 << 2)  // Last Valid Buffer Completion Interrupt
#define PO_SR_BCIS              (1 << 3)  // Buffer Completion Interrupt Status
#define PO_SR_FIFOE             (1 << 4)  // FIFO Error
#define PO_SR_BCIS_MASK         (PO_SR_BCIS | PO_SR_LVBCI)

// PO_CR bits (control) — subset used
#define PO_CR_RUN               (1 << 0)
#define PO_CR_RPBM              (1 << 2)  // Reset PO DMA (bus master reset)
#define PO_CR_LVBIE             (1 << 3)  // LVBCI interrupt enable
#define PO_CR_IOCE              (1 << 4)  // IOC interrupt enable

// Buffer Descriptor List entry (8 bytes)
typedef struct {
    U32 addr;   // Physical address of buffer
    U16 len;    // Buffer length in bytes (low 16 bits)
    U16 flags;  // Bit15 IOC, Bit14 BUP, others reserved
} ATTRIB_PACKED AC97_BDL_ENTRY;

#define AC97_BDL_IOC            (1 << 15)
#define AC97_BDL_BUP            (1 << 14)

#define AC97_BDL_ENTRIES        32
#define AC97_BUF_SIZE           4096
#define AC97_ALIGN              0x100

static AC97_BDL_ENTRY *Ac97Bdl = NULLPTR;
static U8 *Ac97Bufs[AC97_BDL_ENTRIES] = { 0 };
static U8 Ac97Lvi = 0; // last valid index
static volatile U32 Ac97TargetBuffers = 0; // how many descriptors to play (0 = indefinite)
static volatile U32 Ac97PlayedBuffers = 0;
static volatile U8  Ac97Active = 0;

static inline U16 NamRead(U16 reg) {
    return _inw((U16)(AC97_NAM_BASE + reg));
}
static inline VOID NamWrite(U16 reg, U16 val) {
    _outw((U16)(AC97_NAM_BASE + reg), val);
}
static inline U8 NabmRead8(U16 reg) {
    return _inb((U16)(AC97_NABM_BASE + reg));
}
static inline U16 NabmRead16(U16 reg) {
    return _inw((U16)(AC97_NABM_BASE + reg));
}
static inline U32 NabmRead32(U16 reg) {
    return _inl((U16)(AC97_NABM_BASE + reg));
}
static inline VOID NabmWrite8(U16 reg, U8 val) {
    _outb((U16)(AC97_NABM_BASE + reg), val);
}
static inline VOID NabmWrite16(U16 reg, U16 val) {
    _outw((U16)(AC97_NABM_BASE + reg), val);
}
static inline VOID NabmWrite32(U16 reg, U32 val) {
    _outl((U16)(AC97_NABM_BASE + reg), val);
}

// Utility: find AC'97-compatible device and enable bus mastering
static BOOLEAN AC97_DISCOVER_AND_ENABLE(VOID) {
    U32 count = PCI_GET_DEVICE_COUNT();
    for (U32 i = 0; i < count; i++) {
        PCI_DEVICE_ENTRY* dev = PCI_GET_DEVICE(i);
        if (!dev) continue;
        if (dev->header.class_code == PCI_CLASS_MULTIMEDIA &&
            dev->header.subclass  == PCI_SUBCLASS_AUDIO) {
            KDEBUG_PUTS("[AC97] Multimedia/Audio PCI device found\n");
            if (!PCI_ENABLE_BUS_MASTERING(dev->bus, dev->slot, dev->func)) {
                KDEBUG_PUTS("[AC97] Bus mastering enable FAILED\n");
                return FALSE;
            }
            // BAR0 = NAM (mixer), BAR1 = NABM (bus master) for ICH AC'97
            U32 bar0 = dev->header.bar0;
            U32 bar1 = dev->header.bar1;
            // Mask I/O BAR flag bits similar to RTL8139 pattern
            AC97_NAM_BASE  = (bar0 & ~0x3);
            AC97_NABM_BASE = (bar1 & ~0x3);
            ac97_dev = dev;

            // IRQ line
            AC97_IRQ = PCI_READ8(dev->bus, dev->slot, dev->func, PCI_INTERRUPT_LINE_OFFSET);
            KDEBUG_PUTS("[AC97] NAM/NABM set, IRQ captured\n");
            return TRUE;
        }
    }
    return FALSE;
}

BOOLEAN AC97_STATUS(VOID) {
    return (ac97_dev != NULLPTR) && AC97_NAM_BASE && AC97_NABM_BASE;
}

// Minimal IRQ handler stub: acknowledge PIC only for now
VOID AC97_HANDLER(U32 vector, U32 errcode) {
    (void)errcode;
    // Clear PO status bits if available; acknowledge PIC
    if (AC97_STATUS()) {
        U16 sr = NabmRead16(NABM_PO_SR);
        if (sr) {
            // Count buffer completions if active
            if (Ac97Active && (sr & (PO_SR_BCIS | PO_SR_LVBCI))) {
                Ac97PlayedBuffers++;
                if (Ac97TargetBuffers && Ac97PlayedBuffers >= Ac97TargetBuffers) {
                    // Target reached — stop
                    U8 cr = NabmRead8(NABM_PO_CR);
                    cr &= (U8)~PO_CR_RUN;
                    NabmWrite8(NABM_PO_CR, cr);
                    Ac97Active = 0;
                }
            }
            // Writing back set bits clears them on ICH
            NabmWrite16(NABM_PO_SR, sr);
        }
    }
    pic_send_eoi((U8)(vector - PIC_REMAP_OFFSET));
}

static VOID Ac97SmallDelay(VOID) {
    // simple delay using I/O, no PIT dependency
    for (volatile U32 i = 0; i < 100000; i++) {
        ASM_VOLATILE("nop");
    }
}

static VOID Ac97CodecReset(VOID) {
    // Cold reset via NAM reset register: write 0, wait, then read
    NamWrite(NAM_RESET, 0x0000);
    Ac97SmallDelay();
    (void)NamRead(NAM_RESET);
}

static BOOLEAN Ac97PowerUp(VOID) {
    // Clear powerdown: write 0 to powerdown control/status and poll ready bit(s)
    NamWrite(NAM_POWERDOWN_CTRL_STAT, 0x0000);
    for (U32 i = 0; i < 100000; i++) {
        U16 v = NamRead(NAM_POWERDOWN_CTRL_STAT);
        // Many codecs set bit 0 (ADC ready), bit 1 (DAC ready), bit 2 (ANL ready), bit 3 (Vref)
        if ((v & 0x000F) == 0x000F) {
            return TRUE;
        }
    }
    return FALSE;
}

static VOID Ac97Set48kAndUnmute(VOID) {
    // Set sample rate to 48000 Hz (if extended audio rate supported)
    NamWrite(NAM_FRONT_DAC_RATE, 48000);
    // Unmute and set volumes to near-max (0x0000 often means 0dB, unmuted)
    NamWrite(NAM_MASTER_VOLUME, 0x0000);
    NamWrite(NAM_PCM_OUT_VOLUME, 0x0000);
}

static BOOLEAN Ac97AllocDma(VOID) {
    Ac97Bdl = (AC97_BDL_ENTRY*)KMALLOC_ALIGN(sizeof(AC97_BDL_ENTRY) * AC97_BDL_ENTRIES, AC97_ALIGN);
    if (!Ac97Bdl) return FALSE;
    MEMZERO(Ac97Bdl, sizeof(AC97_BDL_ENTRY) * AC97_BDL_ENTRIES);

    for (U32 i = 0; i < AC97_BDL_ENTRIES; i++) {
        Ac97Bufs[i] = (U8*)KMALLOC_ALIGN(AC97_BUF_SIZE, AC97_ALIGN);
        if (!Ac97Bufs[i]) return FALSE;
        MEMZERO(Ac97Bufs[i], AC97_BUF_SIZE);
        Ac97Bdl[i].addr = (U32)(Ac97Bufs[i]);
        Ac97Bdl[i].len  = (U16)(AC97_BUF_SIZE & 0xFFFF);
        Ac97Bdl[i].flags = AC97_BDL_IOC; // interrupt on completion
    }

    // Program BDL base to NABM
    NabmWrite32(NABM_PO_BDBAR, (U32)Ac97Bdl);
    // Reset CIV/LVI and clear status
    NabmWrite8(NABM_PO_CIV, 0);
    Ac97Lvi = (U8)(AC97_BDL_ENTRIES - 1);
    NabmWrite8(NABM_PO_LVI, Ac97Lvi);
    NabmWrite16(NABM_PO_SR, NabmRead16(NABM_PO_SR)); // clear latched bits
    return TRUE;
}

BOOLEAN AC97_INIT(VOID) {
    if (!AC97_DISCOVER_AND_ENABLE()) {
        return FALSE;
    }

    if (!AC97_STATUS()) {
        return FALSE;
    }

    // Reset and power up codec
    Ac97CodecReset();
    KDEBUG_PUTS("[AC97] Codec reset\n");
    if (!Ac97PowerUp()) {
        // Continue anyway; some codecs report differently
        KDEBUG_PUTS("[AC97] Power-up maybe incomplete\n");
    }
    Ac97Set48kAndUnmute();
    KDEBUG_PUTS("[AC97] 48kHz + unmute\n");

    // Allocate DMA resources and program BDL
    if (!Ac97AllocDma()) {
        KDEBUG_PUTS("[AC97] DMA alloc FAILED\n");
        return FALSE;
    }
    KDEBUG_PUTS("[AC97] DMA alloc OK\n");

    // Install IRQ handler if IRQ is routed
    if (AC97_IRQ) {
        ISR_REGISTER_HANDLER(PIC_REMAP_OFFSET + AC97_IRQ, AC97_HANDLER);
        PIC_Unmask(AC97_IRQ);
        KDEBUG_PUTS("[AC97] IRQ installed\n");
    }

    // Enable interrupts (IOC/LVB) and start in stopped state; RUN set later when we queue audio
    U8 cr = NabmRead8(NABM_PO_CR);
    cr &= (U8)~PO_CR_RUN;
    cr |= (PO_CR_IOCE | PO_CR_LVBIE);
    NabmWrite8(NABM_PO_CR, cr);
    KDEBUG_PUTS("[AC97] CR set, interrupts enabled\n");

    return TRUE;
}

VOID AC97_STOP(VOID) {
    if (!AC97_STATUS()) return;
    U8 cr = NabmRead8(NABM_PO_CR);
    cr &= (U8)~PO_CR_RUN;
    NabmWrite8(NABM_PO_CR, cr);
    NabmWrite16(NABM_PO_SR, NabmRead16(NABM_PO_SR)); // clear status
    Ac97Active = 0;
    Ac97PlayedBuffers = 0;
    Ac97TargetBuffers = 0;
}

BOOLEAN AC97_PLAY(const U16* pcm, U32 frames, U8 channels, U32 rate) {
    if (!AC97_STATUS()) return FALSE;
    if (channels != 2) return FALSE; // only stereo supported initially

    if (rate) {
        NamWrite(NAM_FRONT_DAC_RATE, (U16)rate);
    }

    U32 bytes_total = frames * 2 /*bytes*/ * channels;
    U32 offset = 0;

    // Stop any ongoing playback and reset counters
    AC97_STOP();
    Ac97PlayedBuffers = 0;
    Ac97TargetBuffers = 0;
    Ac97Active = 0;

    // Determine number of buffers to use
    U32 full_bufs = (bytes_total / AC97_BUF_SIZE);
    U32 tail = (bytes_total % AC97_BUF_SIZE);
    U32 buffers_to_use = full_bufs + (tail ? 1 : 0);
    if (buffers_to_use == 0) buffers_to_use = AC97_BDL_ENTRIES; // tone mode
    // Clamp to available descriptor count to avoid invalid LVI values
    if (buffers_to_use > AC97_BDL_ENTRIES) buffers_to_use = AC97_BDL_ENTRIES;

    // Fill BDL buffers
    for (U32 i = 0; i < AC97_BDL_ENTRIES; i++) {
        U32 to_copy = AC97_BUF_SIZE;
        if (bytes_total && (i < buffers_to_use)) {
            if (offset + to_copy > bytes_total) to_copy = bytes_total - offset;
        } else {
            to_copy = AC97_BUF_SIZE; // tone/idle fill
        }

        if (pcm && bytes_total > 0) {
            MEMCPY((VOIDPTR)Ac97Bufs[i], (const VOIDPTR)(((const U8*)pcm) + offset), to_copy);
            // If fewer than buffer size, zero the rest
            if (to_copy < AC97_BUF_SIZE) {
                MEMZERO(Ac97Bufs[i] + to_copy, AC97_BUF_SIZE - to_copy);
            }
        } else {
            // Generate simple 440 Hz square wave
            U32 samples = AC97_BUF_SIZE / 2; // 16-bit mono samples count
            // we need stereo -> write L,R
            U16 amp = 8000;
            U32 period = (rate ? rate : 48000) / 440;
            if (period == 0) period = 1;
            for (U32 s = 0; s < samples; s += 2) {
                U16 val = (((s / 2) % period) < (period / 2)) ? amp : (U16)(-amp);
                ((U16*)Ac97Bufs[i])[s + 0] = val; // left
                ((U16*)Ac97Bufs[i])[s + 1] = val; // right
            }
        }

        Ac97Bdl[i].len = (U16)to_copy;
        Ac97Bdl[i].flags = AC97_BDL_IOC;

        if (bytes_total == 0) {
            // tone mode; keep filling all buffers
        } else {
            offset += to_copy;
            if (offset >= bytes_total) {
                // mark remaining buffers as silent
                for (U32 j = i + 1; j < AC97_BDL_ENTRIES; j++) {
                    MEMZERO(Ac97Bufs[j], AC97_BUF_SIZE);
                    Ac97Bdl[j].len = (U16)AC97_BUF_SIZE;
                    Ac97Bdl[j].flags = AC97_BDL_IOC;
                }
                break;
            }
        }
    }

    // Program LVI to last descriptor and start RUN
    if (bytes_total) {
        Ac97TargetBuffers = buffers_to_use;
        Ac97Lvi = (U8)(buffers_to_use ? (buffers_to_use - 1) : (AC97_BDL_ENTRIES - 1));
    } else {
        Ac97Lvi = (U8)(AC97_BDL_ENTRIES - 1);
    }
    NabmWrite8(NABM_PO_LVI, Ac97Lvi);

    U8 cr = NabmRead8(NABM_PO_CR);
    cr |= PO_CR_RUN | PO_CR_IOCE | PO_CR_LVBIE;
    NabmWrite8(NABM_PO_CR, cr);
    Ac97Active = 1;

    return TRUE;
}

BOOLEAN AC97_TONE(U32 freq, U32 duration_ms, U32 rate, U16 amp) {
    if (!AC97_STATUS()) return FALSE;
    if (!rate) rate = 48000;
    if (amp == 0) amp = 8000;

    NamWrite(NAM_FRONT_DAC_RATE, (U16)rate);

    // Compute how many bytes are needed for the duration
    U32 frames_needed = (duration_ms * rate) / 1000; // stereo frames
    U32 bytes_total = frames_needed * 4; // 2 ch * 2 bytes
    U32 offset = 0;

    // Stop any ongoing playback and reset counters
    AC97_STOP();
    Ac97PlayedBuffers = 0;
    Ac97TargetBuffers = 0;
    Ac97Active = 0;

    U32 full_bufs = (bytes_total / AC97_BUF_SIZE);
    U32 tail = (bytes_total % AC97_BUF_SIZE);
    U32 buffers_to_use = full_bufs + (tail ? 1 : 0);
    if (duration_ms == 0 || bytes_total == 0) {
        buffers_to_use = AC97_BDL_ENTRIES; // indefinite
    }
    if (buffers_to_use > AC97_BDL_ENTRIES) buffers_to_use = AC97_BDL_ENTRIES;

    for (U32 i = 0; i < AC97_BDL_ENTRIES; i++) {
        // Fill each buffer with a square wave at freq
        U32 samples = AC97_BUF_SIZE / 2; // 16-bit samples count
        U32 period = (freq ? rate / freq : rate / 440);
        if (period == 0) period = 1;
        for (U32 s = 0; s < samples; s += 2) {
            U16 val = (((s / 2) % period) < (period / 2)) ? amp : (U16)(-amp);
            ((U16*)Ac97Bufs[i])[s + 0] = val; // left
            ((U16*)Ac97Bufs[i])[s + 1] = val; // right
        }
        U32 len = AC97_BUF_SIZE;
        if (bytes_total && (i < buffers_to_use)) {
            if (offset + len > bytes_total) len = bytes_total - offset;
        }
        Ac97Bdl[i].len = (U16)len;
        Ac97Bdl[i].flags = AC97_BDL_IOC;

        if (bytes_total > 0) offset += len;
    }

    // Kick the DMA engine
    if (bytes_total) {
        Ac97TargetBuffers = buffers_to_use;
        Ac97Lvi = (U8)(buffers_to_use ? (buffers_to_use - 1) : (AC97_BDL_ENTRIES - 1));
    } else {
        Ac97Lvi = (U8)(AC97_BDL_ENTRIES - 1);
    }
    NabmWrite8(NABM_PO_LVI, Ac97Lvi);
    U8 cr = NabmRead8(NABM_PO_CR);
    cr |= PO_CR_RUN | PO_CR_IOCE | PO_CR_LVBIE;
    NabmWrite8(NABM_PO_CR, cr);
    Ac97Active = 1;
    return TRUE;
}

/* startup melody was removed per request */
