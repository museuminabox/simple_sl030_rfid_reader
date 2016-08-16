/* Find the first text record from an NDEF-formatted Ultralight/NTAG2xx tag
   and write the text to stdout
*/

#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Set this to 1 in order to get additional output of what's
   going on.  Set to 0 to disable spurious output */
#define DEBUG	0

#define SL030_ID	0x50
#define SL030_CMD_SELECT	0x01
#define SL030_READ_PAGE	0x10
#define MAX_TAG_ID_LENGTH	7

#define CMD_BUF_LEN	11
int cmdBufferLen = CMD_BUF_LEN;
unsigned char cmdBuffer[CMD_BUF_LEN]; 

/* Buffer to hold the contents of the RFID tag */
#define kContentsBufferLen 2048
uint8_t gContentsBuffer[kContentsBufferLen];

struct s_cmdResponse
{
  unsigned char iLength;
  unsigned char iCommand;
  unsigned char iStatus;
  unsigned char iTag[MAX_TAG_ID_LENGTH];
  unsigned char iTagType;
};

// Constants to make parsing NDEF records easier
#define NDEF_RECORD_MESSAGE_BEGIN (1<<7)
#define NDEF_RECORD_MESSAGE_END (1<<6)
#define NDEF_RECORD_CHUNK_FLAG (1<<5)
#define NDEF_RECORD_SHORT_RECORD (1<<4)
#define NDEF_RECORD_ID_LENGTH_PRESENT (1<<3)
#define NDEF_RECORD_TNF_MASK 0x07
#define NDEF_TNF_EMPTY 0x00
#define NDEF_TNF_NFC_WELL_KNOWN	0x01
#define NDEF_TNF_MEDIA_TYPE_RFC2046 0x02
#define NDEF_TNF_URI_RFC3986 0x03
#define NDEF_TNF_NFC_EXTERNAL 0x04
#define NDEF_TNF_NFC_EXTERNAL 0x04
// NDEF well-known record types
#define NDEF_RTD_TEXT 'T'
#define NDEF_RTD_URI 'U'

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int readPage(int aI2CDevice, char aPage, unsigned char* aTagBuffer, int* aTagBufferLen)
{
  int ret = BCM2835_I2C_REASON_ERROR_NACK; // Just needs to be not BCM2835_I2C_REASON_OK
  int i;
  struct s_cmdResponse* resp;

  /* Set up command to select a tag */
  cmdBuffer[0] = 2;
  cmdBuffer[1] = SL030_READ_PAGE;
  cmdBuffer[2] = aPage;
  int tries = 0;
  while ((tries++ < 12) && (ret != BCM2835_I2C_REASON_OK))
  {
    bcm2835_i2c_begin();
  
    bcm2835_i2c_setSlaveAddress(SL030_ID);
    ret = bcm2835_i2c_write(cmdBuffer, 3);
#if DEBUG
    printf("write returned %d\n", ret);
#endif
    usleep(12000);
    memset(cmdBuffer, 0, CMD_BUF_LEN);
    ret = bcm2835_i2c_read(cmdBuffer, cmdBufferLen);
    bcm2835_i2c_end();
  }
#if DEBUG
  printf("read returned %d\n", ret);
  for (i = 0; i < cmdBufferLen; i++)
  {
    printf("%02x ", cmdBuffer[i]);
  }
  printf("\n");
#endif
  resp = (struct s_cmdResponse*)cmdBuffer;
#if DEBUG
  printf("Length: %d\n", resp->iLength);
  printf("Status: %d\n", resp->iStatus);
  printf("Status: %u\n", (char)resp->iStatus);
#endif
  if ((resp->iStatus == 0) && (resp->iLength > 2))
  {
    /* We found a tag! */
    /* Copy the ID across */
    /* drop 2 bytes from iLength: one each for command and status */
    *aTagBufferLen = (resp->iLength - 2 > MAX_TAG_ID_LENGTH ? MAX_TAG_ID_LENGTH : resp->iLength-2);
    memcpy(aTagBuffer, resp->iTag, *aTagBufferLen);
    return 1;
  }
  else
  {
    return 0;
  }
}

int checkForTag(int aI2CDevice, unsigned char* aTagBuffer, int* aTagBufferLen)
{
  int ret = 0;
  int i;
  struct s_cmdResponse* resp;

  /* Set up command to select a tag */
  cmdBuffer[0] = 1;
  cmdBuffer[1] = SL030_CMD_SELECT;
  bcm2835_i2c_begin();

  bcm2835_i2c_setSlaveAddress(SL030_ID);
  ret = bcm2835_i2c_write(cmdBuffer, 2);
#if DEBUG
  printf("write returned %d\n", ret);
#endif
  usleep(20000);
  memset(cmdBuffer, 0, CMD_BUF_LEN);
  ret = bcm2835_i2c_read(cmdBuffer, cmdBufferLen);
  bcm2835_i2c_end();
#if DEBUG
  printf("read returned %d\n", ret);
  for (i = 0; i < cmdBufferLen; i++)
  {
    printf("%02x ", cmdBuffer[i]);
  }
  printf("\n");
#endif
  resp = (struct s_cmdResponse*)cmdBuffer;
#if DEBUG
  printf("Length: %d\n", resp->iLength);
  printf("Status: %d\n", resp->iStatus);
  printf("Status: %u\n", (char)resp->iStatus);
#endif
  if ((resp->iStatus == 0) && (resp->iLength > 2))
  {
    /* We found a tag! */
    /* Copy the ID across */
    /* drop 3 bytes from iLength: one each for command, status and tag type */
    *aTagBufferLen = (resp->iLength - 3 > MAX_TAG_ID_LENGTH ? MAX_TAG_ID_LENGTH : resp->iLength-3);
    memcpy(aTagBuffer, resp->iTag, *aTagBufferLen);
    return 1;
  }
  else
  {
    return 0;
  }
}

int main(void)
{
  int ret;
  int fd;
  int tries = 0;
  int tagIdLen = MAX_TAG_ID_LENGTH;
  unsigned char tagId[MAX_TAG_ID_LENGTH];

  bcm2835_init();

  // We're assuming the caller has made sure there's a tag present

  int page = 4; // skip the first four pages as they hold general info on the tag
  int pageLen = MAX_TAG_ID_LENGTH;
  int contentIdx = 0; // where in the content buffer we're up to
  while ((contentIdx < kContentsBufferLen) && readPage(fd, page, &gContentsBuffer[contentIdx], &pageLen))
  {
    contentIdx += pageLen;
    page++; // move onto the next page
  }
#if DEBUG
  printf("Read in %d bytes\n", contentIdx);
  int i;
  for (i = 0; i < contentIdx; i++)
  {
    printf("%02x ", gContentsBuffer[i]);
    if (i % 16 == 15)
    {
      printf("\n");
    }
  }
  printf("\n");
#endif
  // Parse the content to look for NDEF records
  int idx = 0;
  while (idx < contentIdx)
  {
    // Look for the initial TLV structure
    uint8_t t = gContentsBuffer[idx++];
    if (t != 0)
    {
      // It's not a NULL TLV
#if DEBUG
      printf("t: %02x\n", t);
#endif
      int l = gContentsBuffer[idx++];
      idx = MIN(idx, contentIdx);
      if (l == 0xff)
      {
        // 3-byte length format, so the next two bytes are the actual length
        l = gContentsBuffer[idx++] << 8 | gContentsBuffer[idx++];
        idx = MIN(idx, contentIdx);
      }
      uint8_t tnf_byte;
      uint8_t typeLength =0;
      uint32_t payloadLength =0;
      uint8_t idLength =0;
      int messageEnd = idx + l;
      switch (t)
      {
      case 0x03:
        // We've found an NDEF message
        // Parse out each record in it
#if DEBUG
        printf("NDEF message found\n", t);
#endif
        while ((idx < messageEnd) && (idx < contentIdx))
        {
          tnf_byte = gContentsBuffer[idx++];
          idx = MIN(idx, contentIdx);
          typeLength = gContentsBuffer[idx++];
          idx = MIN(idx, contentIdx);
          if (tnf_byte & NDEF_RECORD_SHORT_RECORD)
          {
            payloadLength = gContentsBuffer[idx++];
          }
          else
          {
            payloadLength = (gContentsBuffer[idx++] << 24) |
                            (gContentsBuffer[idx++] << 16) |
                            (gContentsBuffer[idx++] << 8) |
                            (gContentsBuffer[idx++]);
          }
          idx = MIN(idx, contentIdx);
          if (tnf_byte & NDEF_RECORD_ID_LENGTH_PRESENT) {
            idLength = gContentsBuffer[idx++];
            idx = MIN(idx, contentIdx);
          }

#if DEBUG
          printf("NDEF record: tnf_byte: 0x%02x, typeLength: %d, idLength: %d, payloadLength: %d, next byte: 0x%02x\n", tnf_byte, typeLength, idLength, payloadLength, gContentsBuffer[idx]);
#endif
          // Let's see if it's a record we're interested in...
          if ( ((tnf_byte & NDEF_RECORD_TNF_MASK) == NDEF_TNF_NFC_WELL_KNOWN) && 
              (typeLength == 1) && (gContentsBuffer[idx] == NDEF_RTD_TEXT) )
          {
            // It's text!  Let's output it...
            idx += typeLength; // skip over the type
            // skip the language
            int langLength = gContentsBuffer[idx++];
            payloadLength--; // for the langLength byte
            payloadLength -= langLength;
            idx += langLength;
            while (payloadLength-- > 0)
            {
              idx = MIN(idx, contentIdx);
              printf("%c", gContentsBuffer[idx++]);
            }
            // ...and quit
            exit(0);
          }
          else
          {
            // Skip this message
#if DEBUG
            printf("Skipping (tnf_byte: %02x)\n", tnf_byte);
            printf("idx: %d messageEnd: %d typeLength: %d idLength: %d payloadLength: %d contentIdx: %d\n", idx, messageEnd, typeLength, idLength, payloadLength, contentIdx);
#endif
            idx += typeLength+idLength+payloadLength;
            idx = MIN(idx, contentIdx);
          }

          if (tnf_byte & NDEF_RECORD_MESSAGE_END)
          {
            break;
          }
        }
        break;
      case 0xfe:
        // Terminator TLV block, give up now
        exit(1);
        break;
      default:
        // Skip to the next TLV
        idx += l;
        break;
      };
    }
  }
}   


