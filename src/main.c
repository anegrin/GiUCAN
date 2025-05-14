//
// CANable firmware
//


#include "stm32f0xx.h"
#include "config.h"

#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "can.h"
#include "slcan.h"
#include "system.h"
#include "led.h"
#include "error.h"
#include "logging.h"

uint32_t now;

int main(void)
{
    // Initialize peripherals
    system_init();
    can_init();
    led_init();
#ifdef ENABLE_USB_PORT
    usb_init();
#endif


    // Storage for status and received message buffer
    CAN_RxHeaderTypeDef rx_msg_header;
    uint8_t rx_msg_data[8] = {0};
    uint8_t msg_buf[SLCAN_MTU];

    while (1)
    {
        now = HAL_GetTick();
        led_process();
#ifdef ENABLE_CAN_AT_BOOT
    can_set_bitrate(CAN_BITRATE);
    can_enable();
#endif
#ifdef SLCAN
        uint8_t processed = cdc_process();
#endif
        can_process();

        // If CAN message receive is pending, process the message
        if (is_can_msg_pending(CAN_RX_FIFO0))
        {
            // If message received from bus, parse the frame
            if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK)
            {
                uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);

                // Transmit message via USB-CDC
                if (msg_len)
                {
                    LOG("%d RX %d\r\n", now, msg_len);
#ifdef ECHO_MODE
                    CAN_TxHeaderTypeDef echoMsgHeader;
                    echoMsgHeader.IDE= rx_msg_header.IDE;
                    echoMsgHeader.RTR = rx_msg_header.RTR;
                    echoMsgHeader.StdId=rx_msg_header.StdId;
                    echoMsgHeader.DLC=rx_msg_header.DLC;
                    echoMsgHeader.ExtId=rx_msg_header.ExtId;
                    can_tx(&echoMsgHeader, rx_msg_data);
#endif
#ifdef SLCAN
                    CDC_Transmit_FS(msg_buf, msg_len);
#endif
                }
            }
        }
    }
}
