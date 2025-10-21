#include <DRIVERS/CMOS/CMOS.h>
#include <CPU/ISR/ISR.h>
#include <CPU/PIC/PIC.h>
#include <STD/MEM.h>
#include <STD/ASM.h>

#define RTC_REG_B_DM     0x04  // Data Mode (1=Binary, 0=BCD)
#define RTC_REG_B_24H    0x02  // Hour Mode (1=24hr, 0=12hr)
#define RTC_REG_B_UIE    0x10  // Update Ended Interrupt Enable
#define RTC_REG_B_PIE    0x40  // Periodic Interrupt Enable 
#define RTC_REG_B_SQWE   0x08  // Square Wave Enable (Used for IRQ8)
#define RTC_REG_B_SET    0x80  // Set/Disable Update Cycle

// Register A Flags
#define RTC_REG_A_UIP    0x80  // Update in Progress
#define RTC_REG_A_RS_DIV 0x20  // Default Rate Selector Divisor (often 0x20)

#define POLLING_TIME    0xFFFFF

static RTC_REG rtc ATTRIB_DATA = { 0 };

static U8 cmos_read_register(U8 reg_addr) {
    // Write the address to port 0x70. Bit 7 is NMI disable (usually 0).
    _outb(CMOS_ADDR_REG, reg_addr);
    // Read the data from port 0x71.
    return _inb(CMOS_DATA_REG);
}

static void cmos_write_register(U8 reg_addr, U8 value) {
    _outb(CMOS_ADDR_REG, reg_addr);
    _outb(CMOS_DATA_REG, value);
}


static U8 bcd_to_binary(U8 bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

static BOOLEAN cmos_wait_for_ready() {
    U32 timeout = POLLING_TIME;
    while (timeout--) {
        if (!(cmos_read_register(RTC_REG_A) & RTC_REG_A_UIP)) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOLEAN cmos_get_date_time(RTC_DATE_TIME* datetime) {
    U8 status_b = cmos_read_register(RTC_REG_B);
    BOOLEAN is_bcd = !(status_b & RTC_REG_B_DM);

    RTC_DATE_TIME current;
    RTC_DATE_TIME last;

    // 1. Initial read while waiting for UIP to clear.
    if (!cmos_wait_for_ready()) return FALSE;
    MEMZERO(&last, sizeof(RTC_DATE_TIME)); // Initialize to guarantee loop starts

    do {
        // Copy last reading to compare later
        MEMCPY(&last, &current, sizeof(RTC_DATE_TIME));

        // Wait for UIP to clear again (safe reading window)
        if (!cmos_wait_for_ready()) return FALSE;

        // 2. Latch and Read all time components consecutively (0x00 to 0x09)
        current.seconds      = cmos_read_register(0x00);
        current.minutes      = cmos_read_register(0x02);
        current.hours        = cmos_read_register(0x04);
        current.weekday      = cmos_read_register(0x06);
        current.day_of_month = cmos_read_register(0x07);
        current.month        = cmos_read_register(0x08);
        current.year         = cmos_read_register(0x09);
        current.century      = cmos_read_register(0x32); // Read century

        // 3. Check if an update occurred during the reading process.
        // We compare the previous read to the current read. If they are different, 
        // a new update might have started, and we must loop.
    } while (MEMCMP(&last, &current, sizeof(RTC_DATE_TIME)) != 0);


    // 4. Format Conversion
    if (is_bcd) {
        current.seconds      = bcd_to_binary(current.seconds);
        current.minutes      = bcd_to_binary(current.minutes);
        current.hours        = bcd_to_binary(current.hours);
        current.day_of_month = bcd_to_binary(current.day_of_month);
        current.month        = bcd_to_binary(current.month);
        current.year         = bcd_to_binary(current.year);
        current.century      = bcd_to_binary(current.century);
    }
    
    // Handle 12-hour mode if needed (not shown for simplicity, assumes 24H or user handles it)
    if (!(status_b & RTC_REG_B_24H) && (current.hours & 0x80)) {
        current.hours = (current.hours & 0x7F) + 12; // Convert PM hours
    }
    
    // 5. Finalize year (e.g., convert 25/08 to 2025/2008)
    current.year = current.century * 100 + current.year;

    MEMCPY(datetime, &current, sizeof(RTC_DATE_TIME));
    return TRUE;
}

VOID RTC_HANDLER(U32 vector, U32 errcode) {
    cmos_get_date_time(&rtc.t);

    cmos_read_register(RTC_REG_C);
    
    pic_send_eoi(vector - PIC_REMAP_OFFSET);
}

BOOLEAN INIT_RTC(VOID) {
    CLI;
    MEMZERO(&rtc, sizeof(RTC_REG));

    NMI_disable();

    U8 status_b = cmos_read_register(RTC_REG_B);
    status_b |= (RTC_REG_B_24H | RTC_REG_B_DM | RTC_REG_B_PIE);
    status_b &= ~(RTC_REG_B_SET | RTC_REG_B_UIE | RTC_REG_B_SQWE); // Ensure SET, UIE, SQWE are clear
    cmos_write_register(RTC_REG_B, status_b);
    rtc.reg_b = status_b;

    U8 status_a = cmos_read_register(RTC_REG_A);
    status_a = (status_a & (RTC_REG_A_UIP | 0xF0)) | 0x06; // Preserve UIP, other bits 4-7, set RS=0x06 (1024 Hz)
    cmos_write_register(RTC_REG_A, status_a);
    rtc.reg_a = status_a;

    cmos_read_register(RTC_REG_C);

    NMI_enable();
    
    ISR_REGISTER_HANDLER(PIC_REMAP_OFFSET + 8, RTC_HANDLER);
    PIC_Unmask(8);
    STI;
    return TRUE;
}

RTC_DATE_TIME GET_SYS_TIME() {
    RTC_DATE_TIME retval;
    CLI;
    MEMCPY(&retval, &rtc.t, sizeof(RTC_DATE_TIME));
    // cmos_get_date_time(&retval);
    STI;
    return retval;
}