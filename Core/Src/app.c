#include "app.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <memory.h>
#include <stm32l0xx_hal_crc.h>
#include <stm32l0xx_hal_flash_ex.h>

extern CRC_HandleTypeDef hcrc;

/* 3 steps:
  1. Write first word
  2. Write second word
  3. Write third word and CRC
  */
struct user_data {
  uint32_t data[31u];
  uint32_t crc;
}; /* 128 bytes */

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

uint32_t crc32_calc(uint32_t *data, uint32_t len)
{
  return HAL_CRC_Calculate(&hcrc, data, len);
}

enum ud_result {
  UD_SUCCESS = 0,
  UD_CRC_FAILED,
  UD_ALREADY,
  UD_ERASE_FAILED,
  UD_WRITE_FAILED,
  UD_WRITE_CRC_FAILED,
  UD_NO_STEP,
  UD_ERROR,
};

static const char *ud_result_to_str(enum ud_result ret)
{
  switch (ret) {
  case UD_SUCCESS:
    return "success";
  case UD_CRC_FAILED:
    return "crc failed";
  case UD_ALREADY:
    return "already";
  case UD_ERASE_FAILED:
    return "erase failed";
  case UD_WRITE_FAILED:
    return "write failed";
  case UD_WRITE_CRC_FAILED:
    return "write crc failed";
  case UD_NO_STEP:
    return "no step";
  case UD_ERROR:
  default:
    return "undefined error";
  }
}

enum ud_result app_ud_read(struct user_data *ud)
{
  enum ud_result ret = UD_ERROR;
  uint32_t crc;
  uint32_t *addr = (uint32_t *)FLASH_USER_DATA_ADDR;

  /* Read data */
  for (uint32_t i = 0; i < 32u; i++) {
    ud->data[i] = *addr++;
  }

  /* Verify CRC */
  crc = crc32_calc(ud->data, 31u);

  if (crc == ud->crc) {
    ret = UD_SUCCESS;
  } else {
    // memset(ud, 0, sizeof(*ud));
    ret = UD_CRC_FAILED;
  }

  return ret;
}

enum ud_result app_ud_eraze(void)
{
  uint32_t page_error = 0u;
  FLASH_EraseInitTypeDef erase_conf;
  HAL_StatusTypeDef erase_ret;

  erase_conf.TypeErase   = FLASH_TYPEERASE_PAGES;
  erase_conf.PageAddress = FLASH_USER_DATA_ADDR;
  erase_conf.NbPages     = 1;

  HAL_FLASH_Unlock();

  erase_ret = HAL_FLASHEx_Erase(&erase_conf, &page_error);

  HAL_FLASH_Lock();

  /* Interpret result */
  return (erase_ret == HAL_OK) ? UD_SUCCESS : UD_ERASE_FAILED;
}

/* Return whether step is done */
bool ud_step_done(uint32_t step, struct user_data *ud)
{
  bool done = ud->crc != 0; /* If CRC is not 0, then data is written */
  switch (step) {
  case 0:
    done |= ud->data[0u] != 0;
  case 1:
    done |= ud->data[1u] != 0;
  case 2:
    done |= ud->data[2u] != 0;
  default:
    done |= false;
    break;
  };

  return done;
}

enum ud_result app_ud_write_step(uint32_t step, uint32_t data)
{
  struct user_data ud;
  enum ud_result ret = UD_SUCCESS;
  HAL_StatusTypeDef prog_ret;

  if (step <= 2) {
    ret = app_ud_read(&ud);
    if (ret != UD_SUCCESS && ret != UD_CRC_FAILED) {
      return ret;
    }

    if (ud_step_done(step, &ud)) {
      return UD_ALREADY;
    }

    HAL_FLASH_Unlock();

    prog_ret = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_WORD, FLASH_USER_DATA_ADDR + step * 4u, data);
    if (prog_ret == HAL_OK) {
      if (step == 2u) {
        /* TODO Read back written then verify to be sure ? */

        /* Write CRC if last step */
        ud.data[2u] = data;
        ud.crc   = crc32_calc(ud.data, 31u);
        prog_ret = HAL_FLASH_Program(
                FLASH_TYPEPROGRAM_WORD, FLASH_USER_DATA_ADDR + 124u, ud.crc);
        if (prog_ret != HAL_OK) {
          ret = UD_WRITE_CRC_FAILED;
        } else {
          ret = UD_SUCCESS;
        }
      } else {
        ret = UD_SUCCESS;
      }
    } else {
      ret = UD_WRITE_FAILED;
    }

    HAL_FLASH_Lock();

  } else {
    ret = UD_NO_STEP;
  }

  return ret;
}

static void hexdump(uint8_t *data, uint32_t len)
{
  for (uint32_t i = 0; i < len; i++) {
    printf("%02x ", data[i]);
    if ((i + 1) % 16 == 0) {
      printf("\n");
    }
  }
  printf("\n");
}

int app_shell(int chr)
{
  if (chr < 0) return chr;

  struct user_data ud;
  enum ud_result ret;

  printf("rx: %hx\n", (uint8_t)chr);

  switch ((char)chr) {
  case 'r':
    ret = app_ud_read(&ud);
    printf("flash read: %s\n", ud_result_to_str(ret));
    hexdump((uint8_t *)&ud, sizeof(ud));
    break;

  case '0':
    ret = app_ud_write_step(0u, 0xAAAAAAAAu);
    printf("flash write step 0: %s\n", ud_result_to_str(ret));
    break;

  case '1':
    ret = app_ud_write_step(1u, 0xBBBBBBBBu);
    printf("flash write step 1: %s\n", ud_result_to_str(ret));
    break;

  case '2':
    ret = app_ud_write_step(2u, 0xCCCCCCCCu);
    printf("flash write step 2: %s\n", ud_result_to_str(ret));
    break;

  case 'e':
    ret = app_ud_eraze();
    printf("flash erase: %s\n", ud_result_to_str(ret));
    break;
  }

  return 0;
}