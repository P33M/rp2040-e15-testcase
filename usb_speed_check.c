/* USB speed check application - Christian Zietz - 2022 */

#include <stdio.h>

#include "pico/stdlib.h"

#include "hardware/vreg.h"
#include "hardware/clocks.h"

#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"


// === USB CALLBACKS AND FUNCTIONS ===
//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

static bool usb_connected = false;

// Invoked when device is mounted
void tud_mount_cb(void)
{
  usb_connected = true;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  usb_connected = false;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;

}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{

}

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
  // nothing to with DATA & ACK stage
  if (stage != CONTROL_STAGE_SETUP) return true;

  switch (request->bmRequestType_bit.type)
  {
    case TUSB_REQ_TYPE_VENDOR:
      switch (request->bRequest)
      {

        case VENDOR_REQUEST_MICROSOFT:
          if ( request->wIndex == 7 )
          {
            // Get Microsoft OS 2.0 compatible descriptor
            uint16_t total_len;
            memcpy(&total_len, desc_ms_os_20+8, 2);

            return tud_control_xfer(rhport, request, (void*) desc_ms_os_20, total_len);
          }else
          {
            return false;
          }
          break;

        default: break;
      }
    break;

    default: break;
  }

  // stall unknown request
  return false;
}

int main() {

    static char txbuffer[8192]; // larger than USB fifo
    static char rxbuffer[1024];
    uint32_t rng, len;
    // Init USB
    board_init();
    tusb_init();
    // max bit stuff for half of it
    memset(txbuffer, 0xff, sizeof(txbuffer)/2);
    while (1)
    {
        tud_task();
        rng = (time_us_32() ^ 0x55) & 0x3f;
        rng = (rng >> 4) | ((rng & 0xf) << 4); // up to 64us
        len = (time_us_32() % 64) | 0x30; // 48-63 bytes
        if (usb_connected && tud_vendor_available()) {
            // read and discard
            tud_vendor_read(rxbuffer, sizeof(rxbuffer));
        }
        
        if (usb_connected && tud_vendor_write_available()) {
            // write as much data as possible
            busy_wait_us(rng);
            len = MIN(len, tud_vendor_write_available());
            tud_vendor_write(txbuffer, len);
        }
    }

    return 0;
}
