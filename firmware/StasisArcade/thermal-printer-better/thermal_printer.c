// thermal_printer.c — BLE thermal printer client for Raspberry Pi Pico W
//
// Original Arduino library: Copyright (c) 2020 BitBank Software / Larry Bank
// Pico W port: 2024  SPDX-License-Identifier: GPL-3.0-or-later
//
// ─── Architecture ────────────────────────────────────────────────────────────
// This is a pure BTstack application.  main() calls btstack_run_loop_execute()
// which never returns.  Everything is driven by two packet handlers:
//
//   handle_hci_event()   — HCI/GAP level: power-on → scan → connect → disconnect
//   handle_gatt_event()  — GATT level: service discovery → char discovery → ready
//
// When the printer characteristic is found (state == TC_READY) the function
// do_printer_work() is called once.  Put your printing code there.
//
// Write pacing:
//   gatt_client_write_value_of_characteristic_without_response() is limited to
//   one in-flight write per connection.  Chunks are queued in a small ring buffer
//   and drained via a btstack_timer — no blocking, no sleep_ms().
// ─────────────────────────────────────────────────────────────────────────────

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

// ── Font data ─────────────────────────────────────────────────────────────────
// Provide these in font.c (same layout as the Arduino library):
//   ucFont[]    8x8 bitmap, 96 chars from 0x20, 8 bytes/char, MSB = left pixel
//   ucBigFont[] 16x32 bitmap, same charset, 64 bytes/char (2 bytes x 32 rows)
extern const uint8_t ucFont[];
extern const uint8_t ucBigFont[];

// ─────────────────────────────────────────────────────────────────────────────
// Printer table
// ─────────────────────────────────────────────────────────────────────────────
#define PRINTER_MTP2         0
#define PRINTER_MTP3         1
#define PRINTER_CAT          2
#define PRINTER_PERIPAGE     3
#define PRINTER_PERIPAGEPLUS 4
#define PRINTER_FOMEMO       5
#define PRINTER_COUNT        6

typedef struct { const char *name; uint8_t type; } PrinterID;
static const PrinterID kPrinters[] = {
    {"MP210",     PRINTER_MTP2},  {"PT-210",    PRINTER_MTP2},
    {"MTP-2",     PRINTER_MTP2},  {"MPT-II",    PRINTER_MTP2},
    {"MPT-3",     PRINTER_MTP3},  {"MPT-3F",    PRINTER_MTP3},
    {"GT01",      PRINTER_CAT},   {"GT02",      PRINTER_CAT},
    {"GB01",      PRINTER_CAT},   {"GB02",      PRINTER_CAT},
    {"GB03",      PRINTER_CAT},   {"YHK-A133",  PRINTER_CAT},
    {"MX06",      PRINTER_CAT},   {"MX10",      PRINTER_CAT},
    {"PeriPage+", PRINTER_PERIPAGEPLUS},
    {"PeriPage_", PRINTER_PERIPAGE},
    {"T02",       PRINTER_FOMEMO},
    {NULL, 0}
};
static const int kWidth[PRINTER_COUNT] = {384,576,384,576,384,384};

// MTP2/MTP3 128-bit UUIDs (little-endian for BTstack)
// Service:  49535343-FE7D-4AE5-8FA9-9FAFD205E455
static const uint8_t kSvcMTP128[] = {
    0x55,0xE4,0x05,0xD2,0xAF,0x9F,0xA9,0x8F,0xE5,0x4A,0x7D,0xFE,0x43,0x53,0x53,0x49};
// Char:     49535343-8841-43F4-A8D4-ECBE34729BB3
static const uint8_t kCharMTP128[] = {
    0xB3,0x9B,0x72,0x34,0xBE,0xEC,0xD4,0xA8,0xF4,0x43,0x41,0x88,0x43,0x53,0x53,0x49};

// 16-bit UUIDs for the other printers
static const uint16_t kSvc16[PRINTER_COUNT]  = {0x18F0,0x18F0,0xAE30,0xFF00,0xFF00,0xFF00};
static const uint16_t kChar16[PRINTER_COUNT] = {0x2AF1,0x2AF1,0xAE01,0xFF02,0xFF02,0xFF02};

// ─────────────────────────────────────────────────────────────────────────────
// CRC-8 and bit-mirror tables (identical to original Arduino library)
// ─────────────────────────────────────────────────────────────────────────────
static const uint8_t kCRC8[256] = {
    0x00,0x07,0x0e,0x09,0x1c,0x1b,0x12,0x15,0x38,0x3f,0x36,0x31,0x24,0x23,0x2a,0x2d,
    0x70,0x77,0x7e,0x79,0x6c,0x6b,0x62,0x65,0x48,0x4f,0x46,0x41,0x54,0x53,0x5a,0x5d,
    0xe0,0xe7,0xee,0xe9,0xfc,0xfb,0xf2,0xf5,0xd8,0xdf,0xd6,0xd1,0xc4,0xc3,0xca,0xcd,
    0x90,0x97,0x9e,0x99,0x8c,0x8b,0x82,0x85,0xa8,0xaf,0xa6,0xa1,0xb4,0xb3,0xba,0xbd,
    0xc7,0xc0,0xc9,0xce,0xdb,0xdc,0xd5,0xd2,0xff,0xf8,0xf1,0xf6,0xe3,0xe4,0xed,0xea,
    0xb7,0xb0,0xb9,0xbe,0xab,0xac,0xa5,0xa2,0x8f,0x88,0x81,0x86,0x93,0x94,0x9d,0x9a,
    0x27,0x20,0x29,0x2e,0x3b,0x3c,0x35,0x32,0x1f,0x18,0x11,0x16,0x03,0x04,0x0d,0x0a,
    0x57,0x50,0x59,0x5e,0x4b,0x4c,0x45,0x42,0x6f,0x68,0x61,0x66,0x73,0x74,0x7d,0x7a,
    0x89,0x8e,0x87,0x80,0x95,0x92,0x9b,0x9c,0xb1,0xb6,0xbf,0xb8,0xad,0xaa,0xa3,0xa4,
    0xf9,0xfe,0xf7,0xf0,0xe5,0xe2,0xeb,0xec,0xc1,0xc6,0xcf,0xc8,0xdd,0xda,0xd3,0xd4,
    0x69,0x6e,0x67,0x60,0x75,0x72,0x7b,0x7c,0x51,0x56,0x5f,0x58,0x4d,0x4a,0x43,0x44,
    0x19,0x1e,0x17,0x10,0x05,0x02,0x0b,0x0c,0x21,0x26,0x2f,0x28,0x3d,0x3a,0x33,0x34,
    0x4e,0x49,0x40,0x47,0x52,0x55,0x5c,0x5b,0x76,0x71,0x78,0x7f,0x6a,0x6d,0x64,0x63,
    0x3e,0x39,0x30,0x37,0x22,0x25,0x2c,0x2b,0x06,0x01,0x08,0x0f,0x1a,0x1d,0x14,0x13,
    0xae,0xa9,0xa0,0xa7,0xb2,0xb5,0xbc,0xbb,0x96,0x91,0x98,0x9f,0x8a,0x8d,0x84,0x83,
    0xde,0xd9,0xd0,0xd7,0xc2,0xc5,0xcc,0xcb,0xe6,0xe1,0xe8,0xef,0xfa,0xfd,0xf4,0xf3};

static const uint8_t kMirror[256] = {
    0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
    0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
    0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
    0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
    0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
    0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
    0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
    0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
    0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
    0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
    0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
    0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
    0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
    0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
    0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
    0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF};

static uint8_t crc8_buf(const uint8_t *d, int n) {
    uint8_t cs = 0; for (int i=0;i<n;i++) cs=kCRC8[cs^d[i]]; return cs;
}

// ─────────────────────────────────────────────────────────────────────────────
// BLE connection state
// ─────────────────────────────────────────────────────────────────────────────
typedef enum {
    TC_OFF = 0,
    TC_W4_SCAN,
    TC_W4_CONNECT,
    TC_W4_SERVICE,
    TC_W4_CHAR,
    TC_READY,
    TC_DONE,
} State;

static State       state      = TC_OFF;
static uint8_t     ptype      = 0xFF;
static char        pname[32];
static bd_addr_t   paddr;
static bd_addr_type_t paddr_type;
static hci_con_handle_t conn = HCI_CON_HANDLE_INVALID;

static gatt_client_service_t        svc;
static gatt_client_characteristic_t data_ch;
static gatt_client_characteristic_t notify_ch;
static gatt_client_notification_t   notify_reg;

static btstack_packet_callback_registration_t hci_reg;

// ─────────────────────────────────────────────────────────────────────────────
// Write queue — ring buffer of (pointer, length, bytes_sent) entries
// Data pointers must stay valid until drained; we malloc() copies in tp_write().
// ─────────────────────────────────────────────────────────────────────────────
#define WQ_DEPTH    128
#define WQ_CHUNK     20   // bytes per ATT write (safe before MTU exchange)
#define WQ_DELAY_MS   4   // inter-chunk gap in ms

typedef struct { uint8_t *buf; uint16_t len; uint16_t sent; } WQEntry;
static WQEntry   wq[WQ_DEPTH];
static int       wq_head = 0, wq_tail = 0;
static btstack_timer_source_t wq_timer;
static bool      wq_timer_armed = false;

static bool wq_empty(void) { return wq_head == wq_tail; }
static bool wq_full(void)  { return ((wq_tail+1)%WQ_DEPTH)==wq_head; }

static void wq_arm(void);   // forward declare

// Drain one chunk from the head of the queue
static void wq_drain(void)
{
    if (wq_empty() || conn == HCI_CON_HANDLE_INVALID) return;
    WQEntry *e = &wq[wq_head];
    uint16_t rem   = e->len - e->sent;
    uint16_t chunk = rem > WQ_CHUNK ? WQ_CHUNK : rem;
    gatt_client_write_value_of_characteristic_without_response(
        conn, data_ch.value_handle, chunk, e->buf + e->sent);
    e->sent += chunk;
    if (e->sent >= e->len) {
        free(e->buf);
        e->buf  = NULL;
        wq_head = (wq_head+1) % WQ_DEPTH;
    }
}

static void wq_timer_cb(btstack_timer_source_t *ts)
{
    wq_drain();
    if (!wq_empty()) {
        btstack_run_loop_set_timer(ts, WQ_DELAY_MS);
        btstack_run_loop_add_timer(ts);
    } else {
        wq_timer_armed = false;
    }
}

static void wq_arm(void)
{
    if (wq_timer_armed || wq_empty()) return;
    wq_timer_armed = true;
    btstack_run_loop_set_timer(&wq_timer, 0); // fire immediately
    btstack_run_loop_add_timer(&wq_timer);
}

// Queue len bytes from data (malloc'd copy so caller can use stack buffers)
static void tp_write(const uint8_t *data, uint16_t len)
{
    if (conn == HCI_CON_HANDLE_INVALID || !data || !len) return;
    if (wq_full()) { printf("[TP] queue full, dropping %u bytes\n", len); return; }
    uint8_t *copy = malloc(len);
    if (!copy) { printf("[TP] OOM\n"); return; }
    memcpy(copy, data, len);
    wq[wq_tail] = (WQEntry){ copy, len, 0 };
    wq_tail = (wq_tail+1) % WQ_DEPTH;
    wq_arm();
}

// ─────────────────────────────────────────────────────────────────────────────
// CAT protocol helpers
// ─────────────────────────────────────────────────────────────────────────────
#define CAT_RETRACT   0xA0
#define CAT_FEED      0xA1
#define CAT_ENERGY    0xAF
#define CAT_DRAWMODE  0xBE

static void cat_d8(uint8_t cmd, uint8_t d) {
    uint8_t b[9]={0x51,0x78,cmd,0,1,0,d,kCRC8[d],0xFF};
    tp_write(b,9);
}
static void cat_d16(uint8_t cmd, uint16_t d) {
    uint8_t b[10]={0x51,0x78,cmd,0,2,0,(uint8_t)d,(uint8_t)(d>>8),0,0xFF};
    b[8]=crc8_buf(b+6,2);
    tp_write(b,10);
}

// ─────────────────────────────────────────────────────────────────────────────
// Graphics helpers
// ─────────────────────────────────────────────────────────────────────────────
static void tp_pre_gfx(int w, int h)
{
    uint8_t b[16];
    if (ptype==PRINTER_CAT) {
        cat_d8(CAT_DRAWMODE, 0);
    } else if (ptype==PRINTER_MTP2||ptype==PRINTER_MTP3||ptype==PRINTER_FOMEMO) {
        b[0]=0x1d;b[1]='v';b[2]='0';b[3]='0';
        b[4]=(uint8_t)((w+7)/8);b[5]=0;b[6]=(uint8_t)h;b[7]=(uint8_t)(h>>8);
        tp_write(b,8);
    } else if (ptype==PRINTER_PERIPAGE||ptype==PRINTER_PERIPAGEPLUS) {
        uint8_t hdr[4]={0x10,0xff,0xfe,0x01}; tp_write(hdr,4);
        uint8_t z[12]={0}; tp_write(z,12);
        b[0]=0x1d;b[1]=0x76;b[2]=0x30;b[3]=0x00;
        b[4]=(uint8_t)((w+7)/8);b[5]=0;b[6]=(uint8_t)h;b[7]=(uint8_t)(h>>8);
        tp_write(b,8);
    }
}

static void tp_scanline(const uint8_t *row, int len)
{
    if (ptype==PRINTER_CAT) {
        uint8_t b[8+72]; // 72 bytes = max row for 576-wide printer
        b[0]=0x51;b[1]=0x78;b[2]=0xa2;b[3]=0;b[4]=(uint8_t)len;b[5]=0;
        for(int i=0;i<len;i++) b[6+i]=kMirror[row[i]];
        b[6+len]=crc8_buf(b+6,len);
        b[7+len]=0xFF;
        tp_write(b, 8+len);
    } else {
        tp_write(row, (uint16_t)len);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Back buffer  (384 px wide × 200 px tall, adjust as needed)
// ─────────────────────────────────────────────────────────────────────────────
#define BB_W  384
#define BB_H  200
#define BB_PITCH ((BB_W+7)/8)
static uint8_t bb[BB_H * BB_PITCH];
static int16_t cx=0, cy=0;
static int bb_wrap=0;

void tp_fill(uint8_t v)                       { memset(bb,v,sizeof(bb)); }
void tp_set_wrap(int w)                       { bb_wrap=w; }
void tp_cursor(int x,int y)                   { cx=(int16_t)x; cy=(int16_t)y; }

void tp_set_pixel(int x,int y,int on) {
    if(x<0||y<0||x>=BB_W||y>=BB_H) return;
    uint8_t m=0x80>>(x&7);
    if(on) bb[y*BB_PITCH+(x>>3)]|=m; else bb[y*BB_PITCH+(x>>3)]&=~m;
}

void tp_draw_line(int x1,int y1,int x2,int y2,int on)
{
    int dx=x2-x1,dy=y2-y1,t,e,xi,yi;
    uint8_t *p,m;
    if(x1<0||x2<0||y1<0||y2<0||x1>=BB_W||x2>=BB_W||y1>=BB_H||y2>=BB_H) return;
    if(abs(dx)>=abs(dy)){
        if(x2<x1){t=x1;x1=x2;x2=t;t=y1;y1=y2;y2=t;dx=-dx;}
        dy=y2-y1; e=dx>>1; yi=dy<0?-1:1; dy=abs(dy);
        p=&bb[y1*BB_PITCH+(x1>>3)]; m=0x80>>(x1&7);
        for(;x1<=x2;x1++){
            if(on)*p|=m; else *p&=~m;
            m>>=1; if(!m){m=0x80;p++;}
            e-=dy; if(e<0){e+=dx; p+=yi>0?BB_PITCH:-BB_PITCH;}
        }
    } else {
        if(y1>y2){t=x1;x1=x2;x2=t;t=y1;y1=y2;y2=t;}
        dx=x2-x1; e=abs(y2-y1)>>1; xi=dx<0?-1:1; dx=abs(dx);
        p=&bb[y1*BB_PITCH+(x1>>3)]; m=0x80>>(x1&7);
        for(;y1<=y2;y1++){
            if(on)*p|=m; else *p&=~m;
            p+=BB_PITCH; e-=dx;
            if(e<0){e+=abs(y2-y1);
                if(xi>0){m>>=1;if(!m){p++;m=0x80;}}
                else     {m<<=1;if(!m){p--;m=0x01;}}
            }
        }
    }
}

void tp_draw_text(int x, int y, const char *s, int font, int inv)
{
    if(x>=0){cx=(int16_t)x;} if(y>=0){cy=(int16_t)y;}
    if(font==0){ // 8x8
        for(int i=0;s[i]&&cx<BB_W;i++){
            uint8_t tmp[8];
            memcpy(tmp,&ucFont[((uint8_t)s[i]-32)*8],8);
            if(inv) for(int k=0;k<8;k++) tmp[k]=~tmp[k];
            uint8_t *d=&bb[cy*BB_PITCH+cx/8];
            for(int r=0;r<8;r++){*d=tmp[r];d+=BB_PITCH;}
            cx+=8; if(cx>=BB_W&&bb_wrap){cx=0;cy+=8;}
        }
    } else { // 16x32
        for(int i=0;s[i]&&cx<BB_W&&cy<BB_H-31;i++){
            const uint8_t *src=&ucBigFont[((uint8_t)s[i]-32)*64];
            uint8_t *d=&bb[cy*BB_PITCH+cx/8];
            for(int r=0;r<32;r++){d[0]=src[0];d[1]=src[1];src+=2;d+=BB_PITCH;}
            cx+=16; if(cx>=BB_W&&bb_wrap){cx=0;cy+=32;}
        }
    }
}

void tp_print_buffer(void)
{
    tp_pre_gfx(BB_W, BB_H);
    for(int y=0;y<BB_H;y++) tp_scanline(&bb[y*BB_PITCH], BB_PITCH);
}

// ─────────────────────────────────────────────────────────────────────────────
// Text printing
// ─────────────────────────────────────────────────────────────────────────────
static uint8_t cat_llen=0;
static char    cat_line[48];

static void cat_flush_line(void)
{
    cat_d8(CAT_DRAWMODE,1);
    for(int row=0;row<8;row++){
        uint8_t b[8+48];
        b[0]=0x51;b[1]=0x78;b[2]=0xA2;b[3]=0;b[4]=cat_llen;b[5]=0;
        for(int i=0;i<cat_llen;i++)
            b[6+i]=kMirror[ucFont[((uint8_t)(cat_line[i]-32))*8+row]];
        b[6+cat_llen]=crc8_buf(b+6,cat_llen);
        b[7+cat_llen]=0xFF;
        tp_write(b, 8+cat_llen);
    }
    cat_llen=0;
}

void tp_print(const char *s)
{
    if(conn==HCI_CON_HANDLE_INVALID||!s) return;
    if(ptype==PRINTER_CAT){
        for(int i=0;s[i];i++){
            if(s[i]=='\n'){if(!cat_llen){cat_line[0]=' ';cat_llen=1;} cat_flush_line();}
            else if(s[i]>=' '){cat_line[cat_llen++]=s[i]; if(cat_llen==48)cat_flush_line();}
        }
        return;
    }
    if(ptype==PRINTER_PERIPAGE||ptype==PRINTER_PERIPAGEPLUS){
        uint8_t pfx[4]={0x10,0xff,0xfe,0x01}; tp_write(pfx,4);
    }
    tp_write((const uint8_t*)s,(uint16_t)strlen(s));
}

void tp_print_line(const char *s) { tp_print(s); tp_print("\n"); }

// ─────────────────────────────────────────────────────────────────────────────
// Printer controls
// ─────────────────────────────────────────────────────────────────────────────
void tp_feed(int lines)
{
    if(conn==HCI_CON_HANDLE_INVALID) return;
    if(ptype==PRINTER_CAT){
        if(lines<0) cat_d8(CAT_RETRACT,(uint8_t)-lines);
        else        cat_d8(CAT_FEED,   (uint8_t) lines);
    } else if(ptype==PRINTER_MTP2||ptype==PRINTER_MTP3||ptype==PRINTER_FOMEMO){
        // No dedicated feed command: send blank 1-byte-wide scanlines
        uint8_t blank[9]={0x1d,'v','0','0',1,0,1,0,0};
        for(int i=0;i<lines;i++) tp_write(blank,9);
    }
}

void tp_align(uint8_t a) {
    if(conn==HCI_CON_HANDLE_INVALID||a>2) return;
    uint8_t b[3]={0x1b,'a',a}; tp_write(b,3);
}

void tp_set_font(int f,int ul,int dw,int dt,int em) {
    if(ptype!=PRINTER_FOMEMO&&ptype!=PRINTER_MTP2&&ptype!=PRINTER_MTP3
       &&ptype!=PRINTER_PERIPAGE&&ptype!=PRINTER_PERIPAGEPLUS) return;
    uint8_t b[8]; int i=0;
    if(ptype==PRINTER_PERIPAGE||ptype==PRINTER_PERIPAGEPLUS)
        {b[i++]=0x10;b[i++]=0xff;b[i++]=0xfe;b[i++]=0x01;}
    b[i++]=0x1b;b[i++]=0x21;
    b[i]=(uint8_t)f;
    if(ul)b[i]|=0x80; if(dw)b[i]|=0x20; if(dt)b[i]|=0x10; if(em)b[i]|=0x08;
    tp_write(b,i+1);
}

void tp_set_energy(int e) { if(ptype==PRINTER_CAT) cat_d16(CAT_ENERGY,(uint16_t)e); }

void tp_qrcode(const char *text, int sz) {
    if(ptype!=PRINTER_FOMEMO&&ptype!=PRINTER_MTP2&&ptype!=PRINTER_MTP3) return;
    int slen=(int)strlen(text)+3;
    uint8_t sizeQ[]={0x1d,0x28,0x6b,3,0,0x31,0x43,(uint8_t)sz};
    uint8_t errQ[] ={0x1d,0x28,0x6b,3,0,0x31,0x45,0x31};
    uint8_t storeQ[]={0x1d,0x28,0x6b,(uint8_t)(slen&0xFF),(uint8_t)(slen>>8),0x31,0x50,0x30};
    uint8_t printQ[]={0x1d,0x28,0x6b,3,0,0x31,0x51,0x30};
    tp_write(sizeQ,sizeof(sizeQ)); tp_write(errQ,sizeof(errQ));
    tp_write(storeQ,sizeof(storeQ)); tp_write((const uint8_t*)text,(uint16_t)slen);
    tp_write(printQ,sizeof(printQ));
}

void tp_barcode(int type,int h,const char *data,int tpos) {
    if(conn==HCI_CON_HANDLE_INVALID||!data) return;
    uint8_t len=(uint8_t)strlen(data),b[128]; int i=0;
    b[i++]=0x1d;b[i++]=0x48;b[i++]=(uint8_t)tpos;
    b[i++]=0x1d;b[i++]=0x68;b[i++]=(uint8_t)h;
    b[i++]=0x1d;b[i++]=0x77;b[i++]=2;
    b[i++]=0x1d;b[i++]=0x6b;b[i++]=(uint8_t)type;
    b[i++]=len; memcpy(b+i,data,len);
    tp_write(b,i+len);
}

// ─────────────────────────────────────────────────────────────────────────────
// APPLICATION ENTRY POINT
// Called once from handle_gatt_event when printer is ready.
// Put your printing logic here.
// ─────────────────────────────────────────────────────────────────────────────
static void do_printer_work(void)
{
    printf("[APP] printer ready: '%s'  type=%d  width=%d\n",
           pname, ptype, kWidth[ptype]);

    // -- Text --
    tp_print_line("Hello from Pico W!");
    tp_print_line("Pure BTstack, no Arduino");

    // -- Graphic --
    tp_fill(0x00);
    tp_draw_line(0,0,BB_W-1,BB_H-1,1);
    tp_draw_line(0,BB_H-1,BB_W-1,0,1);
    tp_draw_text(8,8,"PICO W",1,0);   // FONT_LARGE
    tp_print_buffer();

    tp_feed(8);
}

// ─────────────────────────────────────────────────────────────────────────────
// GATT event handler
// ─────────────────────────────────────────────────────────────────────────────
static void handle_gatt_event(uint8_t pkt_type, uint16_t chan,
                               uint8_t *pkt, uint16_t sz)
{
    UNUSED(pkt_type); UNUSED(chan); UNUSED(sz);
    uint8_t ev = hci_event_packet_get_type(pkt);

    if (state == TC_W4_SERVICE) {
        if (ev == GATT_EVENT_SERVICE_QUERY_RESULT)
            gatt_event_service_query_result_get_service(pkt, &svc);

        else if (ev == GATT_EVENT_QUERY_COMPLETE) {
            if (gatt_event_query_complete_get_att_status(pkt) != ATT_ERROR_SUCCESS
                || svc.start_group_handle == 0) {
                printf("[TP] service not found, disconnecting\n");
                gap_disconnect(conn); return;
            }
            state = TC_W4_CHAR;
            if (ptype==PRINTER_MTP2||ptype==PRINTER_MTP3)
                gatt_client_discover_characteristics_for_service_by_uuid128(
                    handle_gatt_event, conn, &svc, (uint8_t*)kCharMTP128);
            else
                gatt_client_discover_characteristics_for_service_by_uuid16(
                    handle_gatt_event, conn, &svc, kChar16[ptype]);
        }
    }

    else if (state == TC_W4_CHAR) {
        if (ev == GATT_EVENT_CHARACTERISTIC_QUERY_RESULT) {
            gatt_client_characteristic_t ch;
            gatt_event_characteristic_query_result_get_characteristic(pkt, &ch);
            // CAT: 0xAE01=data, 0xAE02=notify
            if (ptype==PRINTER_CAT && ch.uuid16==0xAE02) notify_ch = ch;
            else                                          data_ch   = ch;
        }
        else if (ev == GATT_EVENT_QUERY_COMPLETE) {
            if (data_ch.value_handle == 0) {
                printf("[TP] data characteristic not found\n");
                gap_disconnect(conn); return;
            }
            if (ptype==PRINTER_CAT && notify_ch.value_handle) {
                gatt_client_listen_for_characteristic_value_updates(
                    &notify_reg, handle_gatt_event, conn, &notify_ch);
            }
            state = TC_READY;
            do_printer_work();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// HCI / GAP event handler
// ─────────────────────────────────────────────────────────────────────────────
static void handle_hci_event(uint8_t pkt_type, uint16_t chan,
                              uint8_t *pkt, uint16_t sz)
{
    UNUSED(chan); UNUSED(sz);
    if (pkt_type != HCI_EVENT_PACKET) return;
    uint8_t ev = hci_event_packet_get_type(pkt);

    switch (ev) {

    // BTstack is up — start scanning
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(pkt) != HCI_STATE_WORKING) break;
        printf("[TP] BTstack ready, scanning...\n");
        state = TC_W4_SCAN;
        gap_set_scan_parameters(1 /*active*/, 0x0030, 0x0030);
        gap_start_scan();
        break;

    // Advertisement received — look for a supported printer name
    case GAP_EVENT_ADVERTISING_REPORT: {
        if (state != TC_W4_SCAN) break;
        const uint8_t *adv    = gap_event_advertising_report_get_data(pkt);
        uint8_t        advlen = gap_event_advertising_report_get_data_length(pkt);
        char name[32] = {0};
        ad_context_t ctx;
        for (ad_iterator_init(&ctx,advlen,adv); ad_iterator_has_more(&ctx); ad_iterator_next(&ctx)) {
            uint8_t dt = ad_iterator_get_data_type(&ctx);
            if (dt==BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME ||
                dt==BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME) {
                uint8_t dlen = ad_iterator_get_data_len(&ctx);
                uint8_t copy = dlen < sizeof(name)-1 ? dlen : sizeof(name)-1;
                memcpy(name, ad_iterator_get_data(&ctx), copy);
                break;
            }
        }
        if (!name[0]) break;
        // find_printer_type modifies name in-place (truncates to 9 for PeriPage+XX)
        char tmp[32]; strncpy(tmp,name,sizeof(tmp)-1);
        if (strlen(tmp)>9) tmp[9]='\0';
        uint8_t pt=0xFF;
        for (int i=0; kPrinters[i].name; i++)
            if (strcmp(tmp, kPrinters[i].name)==0) { pt=kPrinters[i].type; break; }
        if (pt >= PRINTER_COUNT) break;
        printf("[TP] found '%s' type=%d\n", name, pt);
        ptype = pt;
        strncpy(pname, name, sizeof(pname)-1);
        gap_event_advertising_report_get_address(pkt, paddr);
        paddr_type = gap_event_advertising_report_get_address_type(pkt);
        state = TC_W4_CONNECT;
        gap_stop_scan();
        gap_connect(paddr, paddr_type);
        break;
    }

    // LE connection complete
    case HCI_EVENT_LE_META:
        if (hci_event_le_meta_get_subevent_code(pkt) != HCI_SUBEVENT_LE_CONNECTION_COMPLETE) break;
        if (state != TC_W4_CONNECT) break;
        {
            uint8_t status = hci_subevent_le_connection_complete_get_status(pkt);
            if (status != ERROR_CODE_SUCCESS) {
                printf("[TP] connect failed 0x%02x, rescanning\n", status);
                state = TC_W4_SCAN;
                gap_start_scan();
                break;
            }
            conn = hci_subevent_le_connection_complete_get_connection_handle(pkt);
            printf("[TP] connected, discovering service...\n");
            memset(&svc,0,sizeof(svc));
            memset(&data_ch,0,sizeof(data_ch));
            memset(&notify_ch,0,sizeof(notify_ch));
            state = TC_W4_SERVICE;
            if (ptype==PRINTER_MTP2||ptype==PRINTER_MTP3)
                gatt_client_discover_primary_services_by_uuid128(
                    handle_gatt_event, conn, (uint8_t*)kSvcMTP128);
            else
                gatt_client_discover_primary_services_by_uuid16(
                    handle_gatt_event, conn, kSvc16[ptype]);
        }
        break;

    // Disconnection
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        conn = HCI_CON_HANDLE_INVALID;
        data_ch.value_handle = 0;
        printf("[TP] disconnected (0x%02x)\n",
               hci_event_disconnection_complete_get_reason(pkt));
        state = TC_DONE;
        // To reconnect automatically: state=TC_W4_SCAN; gap_start_scan();
        break;

    default: break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main(void)
{
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("cyw43_arch_init failed\n");
        return 1;
    }

    // Wire up the write-drain timer (not added to run loop until first write)
    btstack_run_loop_set_timer_handler(&wq_timer, wq_timer_cb);

    // Register HCI handler and turn BT on
    hci_reg.callback = &handle_hci_event;
    hci_add_event_handler(&hci_reg);
    hci_power_control(HCI_POWER_ON);

    // Hand control to BTstack — never returns.
    btstack_run_loop_execute();
}
