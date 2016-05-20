/* Dump the contents of a Mifare Ultralight (or similar, e.g. NTAG2xx)
   tag to stdout
*/

#include <bcm2835.h>
#include <stdio.h>
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

struct s_cmdResponse
{
  unsigned char iLength;
  unsigned char iCommand;
  unsigned char iStatus;
  unsigned char iTag[MAX_TAG_ID_LENGTH];
  unsigned char iTagType;
};


int readPage(int aI2CDevice, char aPage, unsigned char* aTagBuffer, int* aTagBufferLen)
{
  int ret = 0;
  int i;
  struct s_cmdResponse* resp;

  /* Set up command to select a tag */
  cmdBuffer[0] = 2;
  cmdBuffer[1] = SL030_READ_PAGE;
  cmdBuffer[2] = aPage;
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

#if 0 // We're assuming the caller has made sure there's a tag present
  /* Check for the tag a few times before giving up */
  while ((tries < 10) && (!checkForTag(fd, tagId, &tagIdLen)))
  {
    tries++;
    tagIdLen = MAX_TAG_ID_LENGTH;
    usleep(50000);
  }
 
  if (tries < 10)
#endif
  {
    /* We found a tag! */
    int page = 4; // skip the first four pages as they hold general info on the tag
    int pageLen = MAX_TAG_ID_LENGTH;
    unsigned char pageData[MAX_TAG_ID_LENGTH];
    while (readPage(fd, page, pageData, &pageLen))
    {
      int j;
      for (j = 0; j < pageLen; j++)
      {
#if DEBUG
        printf("%02x", pageData[j]);
#else
        printf("%c", pageData[j]);
#endif
      }
      page++; // move onto the next page
    }
  }   
}

