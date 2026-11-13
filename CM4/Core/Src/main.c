/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c  (CM4 core)
  * @brief          : Potentiometer ADC → SRAM4 shared memory → CM7 dashboard
  *
  * Principe :
  *   1. CM4 attend le signal HSEM du CM7 (boot synchronisation)
  *   2. CM4 initialise l'ADC1 en bare-metal (PA0_C = Arduino A0)
  *   3. CM4 écrit la vitesse (0-250 km/h) en SRAM4 @ 0x38000000
  *   4. CM7 lit cette valeur dans Model::tick() et met à jour le dashboard
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    float speed;           // offset 0  — ADC1 potentiomètre
    float temperature;     // offset 4  — DHT11
    float fuel;            // offset 8  — ADC2 potentiomètre
    float gps_lat;         // offset 12 — GPS fictif
    float gps_lon;         // offset 16 — GPS fictif
    uint8_t gps_updated;   // offset 20 — flag GPS
    uint8_t padding[3];    // offset 21 — alignement
    float lidar_cm;        // offset 24 — distance LiDAR en cm (VL53L0X)
} SharedData_t;

volatile SharedData_t* sharedData = (volatile SharedData_t*)0x38000000;

#define DHT11_PORT GPIOJ
#define DHT11_PIN GPIO_PIN_0
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* DUAL_CORE_BOOT_SYNC_SEQUENCE: Define for dual core boot synchronization    */
/*                             demonstration code based on hardware semaphore */
/* This define is present in both CM7/CM4 projects                            */
/* To comment when developping/debugging on a single core                     */
#define DUAL_CORE_BOOT_SYNC_SEQUENCE

#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
UART_HandleTypeDef huart8;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void MX_MDMA_Init(void);
static void MX_ADC2_Init(void);
static void MX_ADC1_Init(void);

void DWT_Delay_us(volatile uint32_t us)
{
    volatile uint32_t count = us * 60;
    while(count--) { __asm("nop"); }
}

void UART8_Init_Direct(void) {
    // TFmini-S : Arduino D0(RX)=PJ8, D1(TX)=PJ9 => UART8 sur STM32H747I-DISCO
    __HAL_RCC_UART8_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();

    __HAL_RCC_UART8_FORCE_RESET();
    __HAL_RCC_UART8_RELEASE_RESET();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9; // PJ8=RX, PJ9=TX
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART8;
    HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

    // 115200 bauds @ 100MHz (APB1, D2 domain) : 100000000 / 115200 = 868
    UART8->CR1 = 0;
    UART8->CR2 = 0;
    UART8->CR3 = USART_CR3_OVRDIS; // Pas de blocage en cas de surcharge
    UART8->BRR = 868;
    UART8->CR1 = USART_CR1_RE | USART_CR1_UE;
}

// Lit un octet sur UART8 avec timeout (en cycles CPU ~200MHz)
static int UART8_ReadByte(uint8_t *out, uint32_t timeout_cycles) {
    uint32_t t = 0;
    while (!(UART8->ISR & USART_ISR_RXNE_RXFNE)) {
        if (++t > timeout_cycles) return 0;
    }
    *out = (uint8_t)(UART8->RDR & 0xFF);
    return 1;
}

float TFmini_ReadDistance_cm(void) {
    uint8_t b;
    uint8_t buf[9];

    // Effacer les erreurs (overrun, framing, etc.)
    UART8->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF;

    // Pas de données disponibles du tout
    if (!(UART8->ISR & USART_ISR_RXNE_RXFNE)) return -1.0f;

    // Chercher le header 0x59 0x59 dans les 50 premiers octets
    for (int sync = 0; sync < 50; sync++) {
        if (!UART8_ReadByte(&b, 30000)) return -1.0f;
        if (b != 0x59) continue;

        if (!UART8_ReadByte(&b, 30000)) return -1.0f;
        if (b != 0x59) continue;

        // Header trouvé ! Lire les 7 octets suivants
        buf[0] = 0x59; buf[1] = 0x59;
        int ok = 1;
        for (int i = 2; i < 9; i++) {
            if (!UART8_ReadByte(&buf[i], 30000)) { ok = 0; break; }
        }
        if (!ok) return -1.0f;

        // Vérifier le checksum
        uint16_t sum = 0;
        for (int i = 0; i < 8; i++) sum += buf[i];
        if ((sum & 0xFF) == buf[8]) {
            uint16_t dist = buf[2] | (buf[3] << 8);
            if (dist > 0 && dist <= 1200) return (float)dist;
        }
    }
    return -1.0f;
}

void DHT11_Start(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    DWT_Delay_us(30);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

uint8_t DHT11_Check_Response(void) {
    uint32_t timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) { if (++timeout > 10000) return 0; }
    timeout = 0;
    while (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) { if (++timeout > 10000) return 0; }
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) { if (++timeout > 10000) return 0; }
    return 1;
}

uint8_t DHT11_Read(void) {
    uint8_t i = 0, j;
    for (j = 0; j < 8; j++) {
        uint32_t timeout = 0;
        while (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) { if (++timeout > 10000) break; }
        DWT_Delay_us(40);
        if (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) { i &= ~(1 << (7 - j)); }
        else { i |= (1 << (7 - j)); timeout = 0; while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) { if (++timeout > 10000) break; } }
    }
    return i;
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

/* USER CODE BEGIN Boot_Mode_Sequence_1 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
  /*HW semaphore Clock enable*/
  __HAL_RCC_HSEM_CLK_ENABLE();
  /* Activate HSEM notification for Cortex-M4*/
  HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));
  /*
  Domain D2 goes to STOP mode (Cortex-M4 in deep-sleep) waiting for Cortex-M7 to
  perform system initialization (system clock config, external memory configuration.. )
  */
  HAL_PWREx_ClearPendingEvent();
  HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFE, PWR_D2_DOMAIN);
  /* Clear HSEM flag */
  __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));

#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_1 */
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */
  UART8_Init_Direct();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_MDMA_Init();
  MX_ADC2_Init();
  MX_ADC1_Init();

  /* USER CODE BEGIN 2 */
  
  float last_temperature = 21.5f; // Valeur de départ pour test visuel
  uint32_t last_dht11_time = 0;
  
  // Initialisation des coordonnées GPS fixes (Sousse, Tunisie) pour test
  sharedData->gps_lat = 35.8256f;
  sharedData->gps_lon = 10.6369f;
  sharedData->gps_updated = 1;

  // Initialiser le LiDAR TFmini-S
  sharedData->lidar_cm = -1.0f;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  float current_speed = 0.0f;
  float current_fuel = 0.0f;

  while (1)
  {
    // --- Read Speed (CH0 on PA0_C via ADC1) ---
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
        current_speed = (float)HAL_ADC_GetValue(&hadc1) * (260.0f / 65535.0f);
    }
    HAL_ADC_Stop(&hadc1);

    // --- Read Fuel (CH1 on PA1_C via ADC2) ---
    HAL_ADC_Start(&hadc2);
    if (HAL_ADC_PollForConversion(&hadc2, 100) == HAL_OK) {
        current_fuel = (float)HAL_ADC_GetValue(&hadc2) * (100.0f / 65535.0f);
    }
    HAL_ADC_Stop(&hadc2);

    // --- Read Temperature (DHT11) ---
    if (HAL_GetTick() - last_dht11_time >= 2000) {
        last_dht11_time = HAL_GetTick();
        DHT11_Start();
        if (DHT11_Check_Response() == 1) {
            uint8_t Rh_byte1 = DHT11_Read();
            uint8_t Rh_byte2 = DHT11_Read();
            uint8_t Temp_byte1 = DHT11_Read();
            uint8_t Temp_byte2 = DHT11_Read();
            uint8_t sum = DHT11_Read();
            if (Rh_byte1 + Rh_byte2 + Temp_byte1 + Temp_byte2 == sum) {
                last_temperature = (float)Temp_byte1;
            }
        }
    }

    sharedData->speed       = current_speed;
    sharedData->fuel        = current_fuel;
    sharedData->temperature = last_temperature;
    sharedData->lidar_cm    = TFmini_ReadDistance_cm();

    HAL_Delay(50);
  }
  /* USER CODE END 3 */
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInitStruct.PLL2.PLL2M = 2;
  PeriphClkInitStruct.PLL2.PLL2N = 12;
  PeriphClkInitStruct.PLL2.PLL2P = 2;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOMEDIUM;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_16B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc1.Init.OversamplingMode = DISABLE;
  hadc1.Init.Oversampling.Ratio = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc2.Init.Resolution = ADC_RESOLUTION_16B;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc2.Init.LowPowerAutoWait = DISABLE;
  hadc2.Init.ContinuousConvMode = ENABLE;
  hadc2.Init.NbrOfConversion = 1;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
  hadc2.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc2.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc2.Init.OversamplingMode = DISABLE;
  hadc2.Init.Oversampling.Ratio = 1;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * Enable MDMA controller clock
  */
static void MX_MDMA_Init(void)
{

  /* MDMA controller clock enable */
  __HAL_RCC_MDMA_CLK_ENABLE();
  /* Local variables */

}

/* MX_UART8_Init supprimé */


/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOJ_CLK_ENABLE(); /* LiDAR sur PJ8/PJ9 */
  __HAL_RCC_GPIOB_CLK_ENABLE(); /* DHT11 sur PB9 (D14) + I2C1 sur PB8/PB9 */
  __HAL_RCC_GPIOA_CLK_ENABLE(); /* ADC sur PA0_C, PA1_C */


  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : DHT11_DATA_Pin */
  GPIO_InitStruct.Pin = DHT11_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DHT11_DATA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PJ8 et PJ9 pour Software UART LiDAR */
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */


  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
