/*
 * canTest.c - Audi TT 8N DIS ("FIS") Fullscreen Test for ColorMFA
 *
 * Implements the verified Audi TT (8N) cluster protocol incorporating findings
 * from pcbbc (TT Forum) and working sketches (sketch_v2 / DIS_Draw):
 *
 *   1. Open channel        6C0: A0 0F 8A FF 4A FF   ->  6C1: A1 0F 8A FF 4A FF
 *   2. Session handshake   15 02 01 02 00 00 -> 00 01
 *                          01 02 00          -> 00 01
 *                          A3 x4
 *                          01 02 01          -> 00 01
 *                          01 01 00
 *                          08                -> 09 ... (cluster ident)
 *                          09 20 00 50 07 12 'NAVLL-0200' 00   (nav ident)
 *                          24
 *                          A3 x4
 *                          28                -> 29 ...
 *                          20 3B A0 00       -> 21 3B A0 00
 *   3. Claim Screen        52 05 82 00 00 40 58 -> 53 8A (Fullscreen Granted!)
 *   4. Draw & Refresh      57 09 02 00 14 "AUDI TT" -> 39 Commit (every 100 ms)
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include "USART0.h"
#include "SJA1000.h"
#include "TJA1054.h"

#define RBS             0x01    /* Receive Buffer Status in CAN_STATUS */
#define DOS             0x02    /* Data Overrun Status in CAN_STATUS */
#define CDO             0x08    /* Clear Data Overrun in CAN_CMD */

/* ---------------- Tunables ---------------- */
#define MAX_FRAME_PAYLOAD   42      /* 8N cluster buffer limit (6 x 7 bytes) */
#define BODY_GAP_MS         5       /* gap between 0x2n body packets */
#define STEP_GAP_MS         10      /* gap between protocol steps */
#define ACK_TIMEOUT_MS      150     /* wait for Bn after a 0x1n packet */
#define MSG_TIMEOUT_MS      300     /* wait for an expected cluster message */
#define REFRESH_RATE_MS     100     /* must be < 250ms to beat cluster watchdog */
#define RESTART_DELAY_MS    1000    /* wait before restarting after a failure */

/* Region flags (52) */
#define RGN_CLAIM       0x80
#define RGN_CLEAR       0x02

/* Write flags (57) */
#define WR_FIXED_RED    0x02        /* fixed font, red on black */

/* Fullscreen defaults (0, 0, 64, 88) */
#define FULL_X          0
#define FULL_Y          0
#define FULL_W          64
#define FULL_H          88

/* ---------------- State ---------------- */
static uint32_t g_ms           = 0;
static uint32_t g_last_nm_ms   = 0;

static uint8_t  g_sn           = 0;     /* next TX sequence number */
static uint8_t  g_ack          = 0xFF;  /* last Bn nibble received (0xFF = none) */
static uint8_t  g_got_a1       = 0;     /* A1 received (answer to our A0 / A3) */
static uint8_t  g_cluster_a0   = 0;     /* cluster opened the channel itself */
static uint8_t  g_region_status = 0;    /* last 53 xx status byte */
static uint8_t  g_quiet        = 0;     /* 1 = suppress raw frame logging */
static uint16_t g_handshakes_ok = 0;

static uint8_t  g_rx_buf[48];           /* reassembled cluster message */
static uint8_t  g_rx_len       = 0;
static uint8_t  g_rx_ready     = 0;     /* complete message waiting in g_rx_msg */
static uint8_t  g_rx_msg[48];
static uint8_t  g_rx_msg_len   = 0;

/* ================================================================ */
/* UART / printf                                                     */
/* ================================================================ */
static int uart_putchar(char c, FILE *stream)
{
    (void)stream;
    if (c == '\n') {
        USART0_Transmit('\r');
    }
    USART0_Transmit((uint8_t)c);
    return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

static void print_bytes(const uint8_t *d, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        printf_P(PSTR(" %02X"), d[i]);
    }
    printf_P(PSTR("\n"));
}

/* ================================================================ */
/* Low level CAN                                                     */
/* ================================================================ */
static void tx_6c0(uint8_t len, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
                   uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
    uint8_t b[8] = { d0, d1, d2, d3, d4, d5, d6, d7 };
    CAN_Transmit(0x6C0, len, d0, d1, d2, d3, d4, d5, d6, d7);
    if (g_quiet) {
        return;
    }
    printf_P(PSTR("%8lu 6C0 TX [%u]:"), g_ms, len);
    print_bytes(b, len);
}

static void send_nav_nm(void)
{
    /* OSEK NM Navigation (0x43B) + mNAV_1 (0x604) NV1_NAV_On = 1 */
    CAN_Transmit(0x43B, 6, 0x08, 0x02, 0xC0, 0x01, 0x00, 0x00, 0, 0);
    CAN_Transmit(0x604, 8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    g_last_nm_ms = g_ms;
}

/* ================================================================ */
/* 0x6C1 receive handling                                            */
/* ================================================================ */
static void handle_6c1(const uint8_t *d, uint8_t len)
{
    if (len < 1) {
        return;
    }
    uint8_t b0 = d[0];
    uint8_t hi = b0 & 0xF0;

    if (b0 == 0xA0) {
        /* Cluster keep-alive / session ping: reply with A1 0F */
        tx_6c0(2, 0xA1, 0x0F, 0, 0, 0, 0, 0, 0);
        g_cluster_a0 = 1;
    } else if (b0 == 0xA1) {
        g_got_a1 = 1;
    } else if (b0 == 0xA3) {
        /* Cluster keep-alive inquiry: reply with A1 0F */
        tx_6c0(2, 0xA1, 0x0F, 0, 0, 0, 0, 0, 0);
    } else if (hi == 0xB0 || hi == 0x90) {
        g_ack = b0 & 0x0F;          /* Bn = ready, 9n = not ready */
    } else if (hi == 0x00 || hi == 0x10 || hi == 0x20 || hi == 0x30) {
        /* Data packet from cluster: reassemble */
        for (uint8_t i = 1; i < len && g_rx_len < sizeof(g_rx_buf); i++) {
            g_rx_buf[g_rx_len++] = d[i];
        }
        if (hi == 0x00 || hi == 0x10) {
            /* Acknowledge next expected sequence number */
            tx_6c0(1, 0xB0 | ((b0 + 1) & 0x0F), 0, 0, 0, 0, 0, 0, 0);
        }
        if (hi == 0x10 || hi == 0x30) {
            /* End of message */
            for (uint8_t i = 0; i < g_rx_len; i++) {
                g_rx_msg[i] = g_rx_buf[i];
            }
            g_rx_msg_len = g_rx_len;
            g_rx_ready   = 1;
            g_rx_len     = 0;
            if (!g_quiet) {
                printf_P(PSTR("         <- cluster msg [%u]:"), g_rx_msg_len);
                print_bytes(g_rx_msg, g_rx_msg_len);
            }
        }
    }
}

/* Drain SJA1000 RX FIFO */
static void rx_poll(void)
{
    uint8_t guard = 32;
    while ((CAN_STATUS & RBS) && guard--) {
        uint16_t id = ((uint16_t)CAN_RX_BUF1 << 3) | (CAN_RX_BUF2 >> 5);
        uint8_t  n  = CAN_RX_FRMINFO & 0x0F;
        if (n > 8) {
            n = 8;
        }
        uint8_t b[8];
        for (uint8_t i = 0; i < n; i++) {
            b[i] = *(volatile uint8_t *)(CAN_RX_DATA_BASE + i);
        }
        CAN_CMD = (CAN_STATUS & DOS) ? (RRB | CDO) : RRB;

        if (id == 0x6C1) {
            if (!g_quiet) {
                printf_P(PSTR("%8lu 6C1 RX [%u]:"), g_ms, n);
                print_bytes(b, n);
            }
            handle_6c1(b, n);
        }
    }
}

static void service_1ms(void)
{
    rx_poll();
    _delay_ms(1);
    g_ms++;
    if ((g_ms - g_last_nm_ms) >= 500) {
        send_nav_nm();
    }
}

static void service_ms(uint16_t ms)
{
    while (ms--) {
        service_1ms();
    }
}

/* ================================================================ */
/* Transport                                                         */
/* ================================================================ */
static uint8_t frame_send(const uint8_t *p, uint8_t len)
{
    uint8_t off = 0;

    if (len == 0 || len > MAX_FRAME_PAYLOAD) {
        return 0;
    }

    g_rx_ready = 0;

    while (1) {
        uint8_t rem  = len - off;
        uint8_t n    = (rem > 7) ? 7 : rem;
        uint8_t last = (rem <= 7);
        uint8_t f[7] = { 0, 0, 0, 0, 0, 0, 0 };
        for (uint8_t i = 0; i < n; i++) {
            f[i] = p[off + i];
        }
        tx_6c0(n + 1, (last ? 0x10 : 0x20) | g_sn, f[0], f[1], f[2], f[3], f[4], f[5], f[6]);
        g_sn = (g_sn + 1) & 0x0F;
        off += n;
        if (last) {
            break;
        }
        service_ms(BODY_GAP_MS);
    }

    g_ack = 0xFF;
    for (uint16_t t = 0; t < ACK_TIMEOUT_MS; t++) {
        service_1ms();
        if (g_ack != 0xFF) {
            if (g_ack != g_sn) {
                g_sn = g_ack;
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

static uint8_t wait_msg(uint8_t b0, uint8_t b1, uint16_t timeout_ms)
{
    while (timeout_ms--) {
        service_1ms();
        if (g_rx_ready) {
            g_rx_ready = 0;
            if (g_rx_msg_len >= 1 && g_rx_msg[0] == b0 &&
                (b1 == 0xFF || (g_rx_msg_len >= 2 && g_rx_msg[1] == b1))) {
                return 1;
            }
        }
    }
    return 0;
}

static uint8_t keepalive(void)
{
    g_got_a1 = 0;
    tx_6c0(1, 0xA3, 0, 0, 0, 0, 0, 0, 0);
    for (uint8_t t = 0; t < 50; t++) {
        service_1ms();
        if (g_got_a1) {
            return 1;
        }
    }
    return 0;
}

static uint8_t step(const char *name_P, const uint8_t *msg, uint8_t len,
                    uint8_t want0, uint8_t want1)
{
    if (!g_quiet) {
        printf_P(PSTR("--- "));
        printf_P(name_P);
        printf_P(PSTR("\n"));
    }
    if (!frame_send(msg, len)) {
        return 0;
    }
    if (want0 != 0xFF) {
        if (!wait_msg(want0, want1, MSG_TIMEOUT_MS)) {
            return 0;
        }
    }
    service_ms(STEP_GAP_MS);
    return 1;
}

/* ================================================================ */
/* Session Setup                                                     */
/* ================================================================ */
static uint8_t open_channel(void)
{
    for (uint8_t attempt = 0; attempt < 8; attempt++) {
        g_got_a1     = 0;
        g_cluster_a0 = 0;
        tx_6c0(6, 0xA0, 0x0F, 0x8A, 0xFF, 0x4A, 0xFF, 0, 0);
        for (uint8_t t = 0; t < 100; t++) {
            service_1ms();
            if (g_got_a1 || g_cluster_a0) {
                if (!g_quiet) {
                    printf_P(PSTR("*** channel open\n"));
                }
                g_sn       = 0;
                g_rx_len   = 0;
                g_rx_ready = 0;
                service_ms(STEP_GAP_MS);
                return 1;
            }
        }
    }
    return 0;
}

static uint8_t init_dis(void)
{
    static const uint8_t m15[]     = { 0x15, 0x02, 0x01, 0x02, 0x00, 0x00 };
    static const uint8_t m01_200[] = { 0x01, 0x02, 0x00 };
    static const uint8_t m01_201[] = { 0x01, 0x02, 0x01 };
    static const uint8_t m01_100[] = { 0x01, 0x01, 0x00 };
    static const uint8_t m08[]     = { 0x08 };
    static const uint8_t m09[]     = { 0x09, 0x20, 0x00, 0x50, 0x07, 0x12,
                                       'N', 'A', 'V', 'L', 'L', '-', '0', '2', '0', '0', 0x00 };
    static const uint8_t m24[]     = { 0x24 };
    static const uint8_t m28[]     = { 0x28 };
    static const uint8_t m20[]     = { 0x20, 0x3B, 0xA0, 0x00 };
    uint8_t fails = 0;

    g_quiet = (g_handshakes_ok > 0);
    if (!g_quiet) {
        printf_P(PSTR("\n===== INIT DIS session =====\n"));
    }
    if (!open_channel()) {
        g_quiet = 0;
        return 0;
    }

    fails += !step(PSTR("15"), m15, sizeof(m15), 0x00, 0x01);
    fails += !step(PSTR("01 02 00"), m01_200, sizeof(m01_200), 0x00, 0x01);
    for (uint8_t i = 0; i < 4; i++) {
        keepalive();
        service_ms(STEP_GAP_MS);
    }
    fails += !step(PSTR("01 02 01"), m01_201, sizeof(m01_201), 0x00, 0x01);
    fails += !step(PSTR("01 01 00"), m01_100, sizeof(m01_100), 0xFF, 0xFF);
    fails += !step(PSTR("08"), m08, sizeof(m08), 0x09, 0xFF);
    fails += !step(PSTR("09 NAVLL"), m09, sizeof(m09), 0xFF, 0xFF);
    fails += !step(PSTR("24"), m24, sizeof(m24), 0xFF, 0xFF);
    for (uint8_t i = 0; i < 4; i++) {
        keepalive();
        service_ms(STEP_GAP_MS);
    }
    fails += !step(PSTR("28"), m28, sizeof(m28), 0x29, 0xFF);
    fails += !step(PSTR("20 3B A0 00"), m20, sizeof(m20), 0x21, 0x3B);

    g_quiet = 0;
    if (fails == 0) {
        g_handshakes_ok++;
    }
    printf_P(PSTR("%8lu INIT done (#%u), %u step(s) failed\n"), g_ms, g_handshakes_ok, fails);
    return (fails == 0);
}

/* Fullscreen Claim: Flags 0x82 (Claim + Clear), X=0, Y=0, W=64, H=88 */
static uint8_t claim_fullscreen(void)
{
    uint8_t m[] = { 0x52, 0x05, (RGN_CLAIM | RGN_CLEAR), FULL_X, FULL_Y, FULL_W, FULL_H };

    printf_P(PSTR("\n===== CLAIM Fullscreen Display =====\n"));
    if (!frame_send(m, sizeof(m))) {
        return 0;
    }
    if (!wait_msg(0x53, 0x8A, 300)) {
        printf_P(PSTR("         !! claim not granted\n"));
        return 0;
    }
    printf_P(PSTR("*** claim GRANTED (53 8A) ***\n"));
    return 1;
}

/* Single-packet text write (matches working sketch_v2.txt) */
static uint8_t draw_single_frame_text(const char *text, uint8_t y_pos)
{
    uint8_t len = 0;
    while (text[len]) {
        len++;
    }
    /* Total command length: cmd(1) + len(1) + flags(1) + x(1) + y(1) + chars(len) */
    uint8_t buf[16];
    buf[0] = 0x57;                      /* 'W' command */
    buf[1] = 3 + len;                   /* rest length */
    buf[2] = WR_FIXED_RED;              /* fixed font, red on black */
    buf[3] = 0;                         /* X = 0 */
    buf[4] = y_pos;                     /* Y coordinate */
    for (uint8_t i = 0; i < len; i++) {
        buf[5 + i] = (uint8_t)text[i];
    }

    if (!frame_send(buf, 5 + len)) {
        return 0;
    }

    /* Send Commit 0x39 */
    static const uint8_t m39[] = { 0x39 };
    return frame_send(m39, sizeof(m39));
}

/* ================================================================ */
/* Main                                                              */
/* ================================================================ */
int main(void)
{
    USART0_Init(BAUD115200);
    CAN_Reg_Init();
    TJA1054_Init();

    GICR &= ~(1 << INT0);  /* Disable INT0 so interrupts don't steal RX frames */
    stdout = &mystdout;
    sei();

    /* 100 kbps Audi Infotainment CAN */
    SJA1000_Init(0x03, 0x4D, READWRITE);
    CAN_INTEN = 0x00;      /* Ensure polled mode */

    printf_P(PSTR("\n==================================================\n"));
    printf_P(PSTR(" CANDIP TT 8N Fullscreen Test (ColorMFA)\n"));
    printf_P(PSTR("==================================================\n"));

    send_nav_nm();
    service_ms(200);

    while (1) {
        if (!init_dis()) {
            printf_P(PSTR("!!! handshake failed - retrying\n"));
            service_ms(RESTART_DELAY_MS);
            continue;
        }

        if (!claim_fullscreen()) {
            printf_P(PSTR("!!! claim failed - retrying\n"));
            service_ms(RESTART_DELAY_MS);
            continue;
        }

        printf_P(PSTR("\n===== RUNNING (Refreshing Display every 100ms) =====\n"));

        /* Continuous refresh loop: strobes text every 100ms to beat the 250ms watchdog */
        uint8_t frame_toggle = 0;
        while (1) {
            uint8_t ok;
            if (frame_toggle) {
                ok = draw_single_frame_text("AUDI TT", 15);
            } else {
                ok = draw_single_frame_text("HELLO", 15);
            }
            frame_toggle ^= 1;

            if (!ok) {
                printf_P(PSTR("!!! write failed - restarting handshake\n"));
                break;
            }

            service_ms(REFRESH_RATE_MS);
        }
    }

    return 0;
}