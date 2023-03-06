#include "app.h"

#include <stdio.h>

#include <stm32l0xx_hal_flash_ex.h>
#include <stm32l0xx_hal_crc.h>

struct user_data {
  uint32_t data[124u];
  uint32_t crc;
};

/*
1 sector is 32 pages of 128 bytes each

Page size is FLASH_PAGE_SIZE

Granularity of:
 - FLASH erase: 1 page
 - FLASH write: 1 word
 - FLASH write protection: 1 sector
*/

/* 6 pages of 128 bytes each to FLASH_END */
#define FLASH_USER_DATA_ADDR 0x0802FD00

#define FLASH_END_ADDR 0x0802FFFF

static CRC_HandleTypeDef hcrc;

static int crc_init(void)
{
  // hcrc.Instance = CRC;
  // hcrc.Lock
}

static int crc_deinit(void)
{
  
}

int crc32_calc(uint32_t *data, uint32_t len)
{
  CRC_HandleTypeDef hcrc;
  HAL_CRC_Init(&hcrc);
}

int app_ud_read(struct user_data *ud)
{
  HAL_FLASH_Unlock();

  HAL_FLASH_Lock();
  // HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_USER_DATA_ADDR, 0x12345678);
}

int app_ud_write(struct user_data *ud)
{

}


int app_shell(int chr)
{
  if (chr < 0) return chr;

  printf("rx: %hx\n", (uint8_t)chr);

  switch ((char)chr) {
    case 'r':
      printf("flash read..\n");
      break;
  }

  return 0;
}