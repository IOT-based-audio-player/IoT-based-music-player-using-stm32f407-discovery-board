/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "audioI2S.h"
#include "MY_CS43L22.h"
#include "wav_player.h"
#include "lcd_i2c.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern ApplicationTypeDef Appli_state;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_spi3_tx;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
const char *songs[3] = {
    "c1.wav",
    "c2.wav",
    "c3.wav"
};
volatile uint8_t song_index = 0;
uint8_t last_button_state = 0;

uint8_t usbReady = 0;

char current_song[17] = "No Song";   // 16x2 LCD safe


/* Player state */
typedef enum {
    PLAYER_STOPPED,
    PLAYER_PLAYING,
    PLAYER_PAUSED
} PlayerState;

PlayerState playerState = PLAYER_STOPPED;

/* UART RX variables */
uint8_t rxByte;
char rxBuf[20];
uint8_t rxIndex = 0;
//command
volatile uint8_t cmd_play = 0;
volatile uint8_t cmd_pause = 0;
volatile uint8_t cmd_next = 0;
volatile uint8_t cmd_prev = 0;
volatile uint8_t cmd_volup = 0;
volatile uint8_t cmd_voldown = 0;


uint8_t volume = 180;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C2_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
void Command_Handler(void);
void LCD_UpdateStatus(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* Optional: LED indication for song switching */
void Update_LED_For_Song(uint8_t index)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

    if(index == 0) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET); // song1
    else if(index == 1) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET); // song2
    else if(index == 2) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET); // song3
}

void UART_SendString(const char *str)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2S3_Init();
  MX_FATFS_Init();
  MX_USB_HOST_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  LCD_Init(&hi2c2);
  LCD_TaskInit();
  LCD_SetLine1("STM32 Audio");
  LCD_SetLine2("Ready");




  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart2, &rxByte, 1);
  CS43_Init(hi2c1, MODE_I2S);
  CS43_SetVolume(180);//0-255
  CS43_Enable_RightLeft(CS43_RIGHT_LEFT);

  audioI2S_setHandle(&hi2s3);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      MX_USB_HOST_Process();
      wavPlayer_process();
      Command_Handler();

      LCD_Task();   // ✅ ADD THIS (non-blocking LCD engine)

      if(Appli_state == APPLICATION_START)
      {
          HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
      }
      else if(Appli_state == APPLICATION_DISCONNECT)
      {
          HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
          f_mount(NULL, (TCHAR const*)"", 0);
      }

      if (Appli_state == APPLICATION_READY && !usbReady)
      {
          FRESULT res = f_mount(&USBHFatFS, (const TCHAR*)USBHPath, 1);
          if (res == FR_OK)
          {
              usbReady = 1;
              UART_SendString("USB MOUNT OK\n");
              Update_LED_For_Song(song_index);

              LCD_SetLine2("USB Ready");   // optional
          }
          else
          {
              UART_SendString("USB MOUNT FAIL\n");
              LCD_SetLine2("USB Error");
          }
      }
  }

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief I2S3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S3_Init(void)
{

  /* USER CODE BEGIN I2S3_Init 0 */

  /* USER CODE END I2S3_Init 0 */

  /* USER CODE BEGIN I2S3_Init 1 */

  /* USER CODE END I2S3_Init 1 */
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_44K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S3_Init 2 */

  /* USER CODE END I2S3_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


void Command_Handler(void)
{
    if(cmd_play)
    {
        wavPlayer_fileSelect(songs[song_index]);
        strncpy(current_song, songs[song_index], 16);
        current_song[16] = '\0';

        wavPlayer_play();
        playerState = PLAYER_PLAYING;

        LCD_UpdateStatus();
        cmd_play = 0;
    }

    if(cmd_pause)
    {
        if(playerState == PLAYER_PLAYING)
        {
            wavPlayer_pause();
            playerState = PLAYER_PAUSED;
        }
        else if(playerState == PLAYER_PAUSED)
        {
            wavPlayer_resume();
            playerState = PLAYER_PLAYING;
        }

        LCD_UpdateStatus();
        cmd_pause = 0;
    }

    if(cmd_next)
    {
        wavPlayer_stop();

        song_index = (song_index + 1) % 3;
        wavPlayer_fileSelect(songs[song_index]);

        strncpy(current_song, songs[song_index], 16);
        current_song[16] = '\0';

        wavPlayer_play();
        playerState = PLAYER_PLAYING;

        Update_LED_For_Song(song_index);
        LCD_UpdateStatus();
        cmd_next = 0;
    }

    if(cmd_prev)
    {
        wavPlayer_stop();

        song_index = (song_index == 0) ? 2 : song_index - 1;
        wavPlayer_fileSelect(songs[song_index]);

        strncpy(current_song, songs[song_index], 16);
        current_song[16] = '\0';

        wavPlayer_play();
        playerState = PLAYER_PLAYING;

        Update_LED_For_Song(song_index);
        LCD_UpdateStatus();
        cmd_prev = 0;
    }

    if(cmd_volup)
    {
        if(volume < 245) volume += 10;
        CS43_SetVolume(volume);

        LCD_UpdateStatus();
        cmd_volup = 0;
    }

    if(cmd_voldown)
    {
        if(volume > 10) volume -= 10;
        CS43_SetVolume(volume);

        LCD_UpdateStatus();
        cmd_voldown = 0;
    }
}



void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (rxByte == '\n' || rxByte == '\r')
        {
            rxBuf[rxIndex] = 0;

            if (!strcmp(rxBuf, "PLAY"))        cmd_play = 1;
            else if (!strcmp(rxBuf, "PAUSE"))  cmd_pause = 1;
            else if (!strcmp(rxBuf, "NEXT"))   cmd_next = 1;
            else if (!strcmp(rxBuf, "PREV"))   cmd_prev = 1;
            else if (!strcmp(rxBuf, "VOLUP"))  cmd_volup = 1;
            else if (!strcmp(rxBuf, "VOLDN"))  cmd_voldown = 1;

            rxIndex = 0;
            memset(rxBuf, 0, sizeof(rxBuf));
        }
        else if (rxIndex < sizeof(rxBuf) - 1)
        {
            rxBuf[rxIndex++] = rxByte;
        }

        HAL_UART_Receive_IT(&huart2, &rxByte, 1);
    }
}
void LCD_UpdateStatus(void)
{
    char line2[17];

    if (playerState == PLAYER_PLAYING)
        snprintf(line2, 17, "PLAY V:%03d", volume);
    else if (playerState == PLAYER_PAUSED)
        snprintf(line2, 17, "PAUSE V:%03d", volume);
    else
        snprintf(line2, 17, "STOP  V:%03d", volume);

    LCD_SetLine1(current_song);
    LCD_SetLine2(line2);
}



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
