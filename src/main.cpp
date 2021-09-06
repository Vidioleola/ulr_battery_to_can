#include <Arduino.h>

#include <SPI.h>
#include <MCP2515.h>

#define VERBOSE true

typedef enum
{
  WAIT_0xFA = 0,
  WAIT_0xFB,
  WAIT_0xFC,
  WAIT_ID_0,
  WAIT_ID_1,
  WAIT_ID_2,
  WAIT_ID_3,
  WAIT_LEN,
  WAIT_DATA_0,
  WAIT_DATA_1,
  WAIT_DATA_2,
  WAIT_DATA_3,
  WAIT_DATA_4,
  WAIT_DATA_5,
  WAIT_DATA_6,
  WAIT_DATA_7
} STATES;

unsigned long g_id = 0;
unsigned long g_len = 0;
STATES g_state = STATES::WAIT_0xFA;

MCP2515 g_can;
CANMSG g_msg;

char g_can_buffer[8];

#define CONSOLE Serial
#define SERIAL_CAN Serial1

#define canSerial (&SERIAL_CAN)

void setup()
{
  // put your setup code here, to run once:
  CONSOLE.begin(115200);

  SERIAL_CAN.begin(115200, SERIAL_8N1, 3, 4);

  SPI.setClockDivider(SPI_CLOCK_DIV8);

  //SPI.begin(14, 12, 13, SLAVESELECT); modified inside initCAN
  if (g_can.initCAN(CAN_BAUD_250K) == 0)
  {
    CONSOLE.println("initCAN() failed");
    delay(2000);
    exit(-1);
  }

  if (g_can.setCANNormalMode(LOW) == 0) //normal mode non single shot
  {
    CONSOLE.println("setCANNormalMode() failed");
    delay(2000);
    exit(-2);
  }
}

void loop()
{
  bool messageRdy = false;
  while (SERIAL_CAN.available())
  {
    uint8_t byte = (uint8_t)SERIAL_CAN.read();
#if VERBOSE
    CONSOLE.printf("0x%02X ", (byte));
#endif
    switch (g_state)
    { //i know, this state machine is extremely code-inefficient. but EASY to undestand for a GMC
    case STATES::WAIT_0xFA:
      if (byte == 0xFA)
      {
        g_id = 0;
        g_len = 0;

        g_state = STATES::WAIT_0xFB;
      }
      break;
    case STATES::WAIT_0xFB:
      if (byte == 0xFB)
      {
        g_state = STATES::WAIT_0xFC;
      }
      else if (byte != 0xFA) //special case if we have 0xFA 0xFA 0xFB 0xFC
      {
        g_state = STATES::WAIT_0xFA;
      }
      break;
    case STATES::WAIT_0xFC:
      if (byte == 0xFC)
      {
        g_state = STATES::WAIT_ID_0;
      }
      else
      {
        g_state = STATES::WAIT_0xFA;
      }
      break;
    case STATES::WAIT_ID_0:
      g_id += (byte << 0 * 8);
      g_state = STATES::WAIT_ID_1;
      break;
    case STATES::WAIT_ID_1:
      g_id += (byte << 1 * 8);
      g_state = STATES::WAIT_ID_2;
      break;
    case STATES::WAIT_ID_2:
      g_id += (byte << 2 * 8);
      g_state = STATES::WAIT_ID_3;
      break;
    case STATES::WAIT_ID_3:
      g_id += (byte << 3 * 8);
      g_state = STATES::WAIT_LEN;
      break;
    case STATES::WAIT_LEN:
      g_len = byte;
      if (g_len > sizeof(g_can_buffer))
        g_len = sizeof(g_can_buffer); //shoud never happen
      if (g_len == 0)
      {
        messageRdy = true;
      }
      else
      {
        g_state = STATES::WAIT_DATA_0;
      }
      break;
    case STATES::WAIT_DATA_0:
      g_can_buffer[0] = byte;

      if (g_len == 1)
      {
        messageRdy = true;
      }
      else
      {
        g_state = STATES::WAIT_DATA_1;
      }
      break;
    case STATES::WAIT_DATA_1:
      g_can_buffer[1] = byte;

      if (g_len == 2)
      {
        messageRdy = true;
      }
      else
      {
        g_state = STATES::WAIT_DATA_2;
      }
      break;
    case STATES::WAIT_DATA_2:
      g_can_buffer[2] = byte;

      if (g_len == 3)
      {
        messageRdy = true;
      }
      else
      {
        g_state = STATES::WAIT_DATA_3;
      }
      break;
    case STATES::WAIT_DATA_3:
      g_can_buffer[3] = byte;

      if (g_len == 4)
      {
        messageRdy = true;
      }
      else
      {
        g_state = STATES::WAIT_DATA_4;
      }
      break;
    case STATES::WAIT_DATA_4:
      g_can_buffer[4] = byte;

      if (g_len == 5)
      {
        messageRdy = true;
      }
      else
      {
        g_state = STATES::WAIT_DATA_5;
      }
      break;
    case STATES::WAIT_DATA_5:
      g_can_buffer[5] = byte;

      if (g_len == 6)
      {
        messageRdy = true;
      }
      else
      {
        g_state = STATES::WAIT_DATA_6;
      }
      break;
    case STATES::WAIT_DATA_6:
      g_can_buffer[6] = byte;

      if (g_len == 7)
      {
        messageRdy = true;
      }
      else
      {
        g_state = STATES::WAIT_DATA_7;
      }
      break;
    case STATES::WAIT_DATA_7:
      g_can_buffer[7] = byte;
      messageRdy = true;
      break;
    }

    if (messageRdy)
    {
#if VERBOSE
      CONSOLE.printf("\nmessage ready with id %lld\n", (long long)g_id);
#endif

      g_msg.adrsValue = g_id;
      g_msg.isExtendedAdrs = true;
      g_msg.rtr = false;
      g_msg.dataLength = g_len;
      for (int i = 0; i < g_len && i < sizeof(g_msg.data); i++)
      {
        g_msg.data[i] = g_can_buffer[i];
      }
      g_can.transmitCANMessage(g_msg, 500);

      messageRdy = false;
      g_state = STATES::WAIT_0xFA;
      g_id = 0;
      g_len = 0;
    }
  }
}