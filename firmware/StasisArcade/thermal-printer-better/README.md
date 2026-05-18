# Thermal Printer — Raspberry Pi Pico W port

Port of [Larry Bank's Thermal_Printer Arduino library](https://github.com/bitbank2/Thermal_Printer)
to the Raspberry Pi Pico W using the **Pico SDK + BTstack** directly — no Arduino core.

---

## Files

| File | Purpose |
|------|---------|
| `thermal_printer.c` | Entire application — BLE state machine + all printer logic |
| `config/btstack_config.h` | Required BTstack compile-time configuration |
| `CMakeLists.txt` | Build system |
| `font.c` | **You supply this** — copy from the Arduino library (see below) |

---

## Font data you need to supply

Create `font.c` with two arrays copied straight from the Arduino library:

```c
// font.c
#include <stdint.h>
const uint8_t ucFont[]    = { /* 8×8 font, 96 chars × 8 bytes */ };
const uint8_t ucBigFont[] = { /* 16×32 font, 96 chars × 64 bytes */ };
```

Both arrays are in the Arduino library's `font.c` / `bigfont.c` files.
The layout is identical — just remove `PROGMEM` and `pgm_read_byte` references.

---

## Build

Requires Pico SDK ≥ 1.5.1 (first release with BTstack BLE support).

```bash
git clone https://github.com/raspberrypi/pico-sdk --recurse-submodules
export PICO_SDK_PATH=/path/to/pico-sdk

mkdir build && cd build
cmake ..
make -j$(nproc)
```

Hold BOOTSEL while plugging in your Pico W, then copy `thermal_printer.uf2`
to the mass-storage drive that appears.

---

## How it works

This is a single-file BTstack application.  **BTstack owns the run loop.**

```
main()
  cyw43_arch_init()
  hci_add_event_handler(&hci_reg)   ← register handle_hci_event
  hci_power_control(HCI_POWER_ON)
  btstack_run_loop_execute()        ← never returns; all work happens in callbacks
```

### State machine

```
TC_OFF
  ↓  BTSTACK_EVENT_STATE / HCI_STATE_WORKING
TC_W4_SCAN
  ↓  GAP_EVENT_ADVERTISING_REPORT matches a known printer name
TC_W4_CONNECT
  ↓  HCI_SUBEVENT_LE_CONNECTION_COMPLETE (success)
TC_W4_SERVICE        ← gatt_client_discover_primary_services_by_uuid[16|128]
  ↓  GATT_EVENT_QUERY_COMPLETE (service found)
TC_W4_CHAR           ← gatt_client_discover_characteristics_for_service_by_uuid[16|128]
  ↓  GATT_EVENT_QUERY_COMPLETE (characteristic found)
TC_READY             ← do_printer_work() called once
  ↓  HCI_EVENT_DISCONNECTION_COMPLETE
TC_DONE
```

### Write pacing

Writes are queued in a ring buffer (`wq[]`).  A `btstack_timer` drains one
20-byte chunk every 4 ms, matching the inter-packet delay from the original
ESP32 path.  **No `sleep_ms()`, no polling loops** — everything happens inside
BTstack's run loop.

### Customising what gets printed

Edit `do_printer_work()` at the bottom of `thermal_printer.c`.  It is called
exactly once after the printer characteristic is discovered.

---

## What changed from the Arduino version

| Arduino | Pico W BTstack |
|---------|---------------|
| `BLEDevice`, `BLEClient`, `BLERemoteService` objects | `hci_con_handle_t` + BTstack GATT client API |
| `BLE.begin()` / `BLE.scanForName()` / `peripheral.connect()` blocking loops | Single `handle_hci_event` callback; `gap_start_scan()` → `gap_connect()` |
| `pRemoteCharacteristicData->writeValue(buf, n, false)` | `gatt_client_write_value_of_characteristic_without_response()` |
| `delay(4)` between write chunks | `btstack_timer` draining a write queue |
| `pgm_read_byte(&arr[n])` / `PROGMEM` | Plain `arr[n]` — Pico has a flat address space |
| Three platform `#ifdef` blocks (ESP32, NANO33, NRF52) | None — single code path |
| Library with public API | Single self-contained application file |

---

## Supported printers

Same set as the original library:

| BLE advertisement name | Type |
|------------------------|------|
| MP210, PT-210, MTP-2, MPT-II | MTP2 (128-bit UUID) |
| MPT-3, MPT-3F | MTP3 (128-bit UUID) |
| GT01, GT02, GB01, GB02, GB03, YHK-A133, MX06, MX10 | CAT (0xAE30) |
| PeriPage+ | PERIPAGEPLUS (0xFF00) |
| PeriPage_ | PERIPAGE (0xFF00) |
| T02 | FOMEMO (0xFF00) |
