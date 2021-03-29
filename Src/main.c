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
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

#include <stdio.h>
#include <string.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

osThreadId defaultTaskHandle;
osThreadId rxTaskHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
void StartDefaultTask(void const * argument);
void StartRxTask(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

int fputc(int ch, FILE *f) {
    ITM_SendChar(ch);
    return (ch);
};

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
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    /* USER CODE BEGIN 2 */

    /* USER CODE END 2 */

    /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
    /* USER CODE END RTOS_MUTEX */

    /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
    /* USER CODE END RTOS_TIMERS */

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */
    /* definition and creation of defaultTask */
    osThreadDef(defaultTask, StartDefaultTask, osPriorityHigh, 0, 512);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

//  osThreadDef(rxTask, StartRxTask, osPriorityNormal, 0, 384);
//  rxTaskHandle = osThreadCreate(osThread(rxTask), NULL);






    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    /* USER CODE END RTOS_THREADS */

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
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

    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

    /* USER CODE BEGIN USART1_Init 0 */

    /* USER CODE END USART1_Init 0 */

    /* USER CODE BEGIN USART1_Init 1 */

    /* USER CODE END USART1_Init 1 */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    char c;
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&(c), 1);

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN USART1_Init 2 */

    /* USER CODE END USART1_Init 2 */

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
    huart2.Init.BaudRate = 115200;
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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */

int g_tx_error = 0;
HAL_StatusTypeDef K_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    HAL_StatusTypeDef status = HAL_OK;
    for(int32_t i = 1; i < 100000; ++i)
    {
        status = HAL_UART_Transmit_IT(huart, pData, Size);
        if(HAL_OK == status)
        {   // ??
            return status;
        }
        else if(HAL_BUSY == status)
        {
            //printf("HAL_UART_Transmit failed. status:%d, gState:0X%x, Lock:%d\r\n", status, huart->gState, huart->Lock);
            if(HAL_UART_STATE_READY == huart->gState && HAL_LOCKED == huart->Lock && i % 50 == 0)
            {   g_tx_error++;
                __HAL_UNLOCK(huart);
                continue;
            }
        }
        else if(HAL_ERROR == status)
        {   // ??????
            return status;
        }
        else if(HAL_TIMEOUT == status)
        {   // ?????????
            continue;
        }
    }
    // ???N?
    return status;
}

void user_send_data_with_delay(char *buffer) {
	K_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer));
  osDelay(200);
}


#define RING_BUFFER_SIZE            (1000)
typedef struct
{
    uint8_t  data[RING_BUFFER_SIZE];
    uint16_t tail;
    uint16_t head;
} uart_ring_buffer_t;

static uart_ring_buffer_t   g_uart_rx_buf = {0};

#define AT_SUCCESS 0
#define AT_FAILED  -1

/**
 * Receive data on a UART interface
 *
 * @param[in]   uart         the UART interface
 * @param[out]  data         pointer to the buffer which will store incoming data
 * @param[in]   expect_size  number of bytes to receive
 * @param[out]  recv_size    number of bytes received
 * @param[in]   timeout      timeout in milisecond, set this value to HAL_WAIT_FOREVER
 *                           if you want to wait forever
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t _uart_recv(void  *uart, void *data, uint32_t expect_size,
                   uint32_t *recv_size, uint32_t timeout)
{
    uint32_t tmp, read_size;
    uint8_t *buf = (uint8_t *)data;
    uint32_t start_time, expired_time;

    start_time = HAL_GetTick();
    *recv_size = 0;

    expect_size =  expect_size > RING_BUFFER_SIZE ? RING_BUFFER_SIZE : expect_size;

    for (;;) {
        read_size = expect_size;

        tmp = 0;
        /* Loop until data received */
        while (read_size--)
        {
            if (g_uart_rx_buf.head != g_uart_rx_buf.tail)
            {
                /* serial data available, so return data to user */
                *buf++ = g_uart_rx_buf.data[g_uart_rx_buf.head++];
                tmp++;

                /* check for ring buffer wrap */
                if (g_uart_rx_buf.head >= RING_BUFFER_SIZE)
                {
                    /* Ring buffer wrap, so reset head pointer to start of buffer */
                    g_uart_rx_buf.head = 0;
                }
            }
        }

        *recv_size += tmp;
        expect_size -= tmp;

        if(expect_size == 0)
        {
            break;
        }

        expired_time = HAL_GetTick() - start_time;
        if (expired_time > timeout)
        {
            return -1;
        }
    }

    return 0;
}

int user_get_data_with_delay(char *buffer, unsigned int length, unsigned int delay) {
    uint32_t recv_size= 0;
    _uart_recv(NULL, buffer, length,&recv_size, del\
	ay);

    char * found = NULL;
    if(strlen(buffer) == 0) {
        return -9;
    }

    found = strstr(buffer, "OK\r\n");
    if(NULL != found) {
        return AT_SUCCESS;
    }
    return AT_FAILED;
}

int n720_normal_check(char *in_cmd) {
    user_send_data_with_delay(in_cmd);
#define BUF_LEN 50
    char buffer[BUF_LEN] = {0};
    int ret = user_get_data_with_delay(buffer, BUF_LEN, 100);
		printf("sent AT cmd %s ret is %d\n", in_cmd, ret);
    return ret;
}

int n720_check_sim_card() {
    user_send_data_with_delay("AT+CPIN?\r\n");
#define BUF_LEN 50
    char buffer[BUF_LEN] = {0};
    int ret = user_get_data_with_delay(buffer, BUF_LEN,100);
    if(strstr(buffer, "CPIN: READY") != NULL) {
        return AT_SUCCESS;
    }
    return AT_FAILED;
}

int n720_check_cgatt() {
    user_send_data_with_delay("AT+CGATT?\r\n");
#define BUF_LEN 50
    char buffer[BUF_LEN] = {0};
    int ret = user_get_data_with_delay(buffer, BUF_LEN,100);
    if(strstr(buffer, "CGATT: 1") != NULL) {
        return AT_SUCCESS;
    }
    return AT_FAILED;
}

int n720_check_creg() {
    user_send_data_with_delay("AT+CREG?\r\n");
#define BUF_LEN 50
    char buffer[BUF_LEN] = {0};
    int ret = user_get_data_with_delay(buffer, BUF_LEN,100);
    if(strstr(buffer, "CREG: 2") != NULL) {
        return AT_SUCCESS;
    }
    return AT_FAILED;
}

int n720_check_mqttonline() {
    char * inbuffer = "AT+MQTTSTATE?\r\n";
    //HAL_UART_Transmit(&huart1, inbuffer, strlen(inbuffer),100 );
    user_send_data_with_delay(inbuffer);

#define BUF_LEN_1 512
    char buffer[BUF_LEN_1] = {0};
    int ret = user_get_data_with_delay(buffer, BUF_LEN_1, 200);

    if(strstr(buffer, "MQTTSTATE: 0") != NULL) {

        return AT_FAILED;
    }
    return AT_SUCCESS;
}

int n720_check_xiic() {
    user_send_data_with_delay("AT+XIIC?\r\n");
#define BUF_LEN_2 30
    char buffer[BUF_LEN_2] = {0};
    int ret = user_get_data_with_delay(buffer, BUF_LEN_2,100);
    if(strstr(buffer, "+XIIC:    1") != NULL) {
        return AT_SUCCESS;
    }
    return AT_FAILED;
}


int g_rx_error=0;
int g_rx_other_error=0;

HAL_StatusTypeDef K_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    HAL_StatusTypeDef status = HAL_OK;
    for(int32_t i = 1; i < 1000; ++i)
    {
#if 1
        uint32_t isrflags   = READ_REG(huart->Instance->SR);
        if((__HAL_UART_GET_FLAG(huart, UART_FLAG_PE))!=RESET)
        {
            __HAL_UART_CLEAR_PEFLAG(huart);
        }
        if((__HAL_UART_GET_FLAG(huart, UART_FLAG_FE))!=RESET)
        {
            __HAL_UART_CLEAR_FEFLAG(huart);
        }
        if((__HAL_UART_GET_FLAG(huart, UART_FLAG_NE))!=RESET)
        {
            __HAL_UART_CLEAR_NEFLAG(huart);
        }
        if((__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE))!=RESET)
        {
            //READ_REG(huart->Instance->CR1);//ORE???,????CR
            //__HAL_UART_CLEAR_FLAG(huart, UART_FLAG_ORE);
            __HAL_UART_CLEAR_OREFLAG(huart);
        }
        if(HAL_UART_ERROR_NONE != huart->ErrorCode)
        {
            huart->ErrorCode = HAL_UART_ERROR_NONE;
					 
        }
#endif
        status = HAL_UART_Receive_IT(huart, pData, Size);
        if(HAL_OK == status)
        {
            return status;
        }
        else if(HAL_BUSY == status)
        {
            //printf("HAL_UART_Receive_IT failed. status:%d, RxState:0X%x, Lock:%d\r\n", status, huart->RxState, huart->Lock);
            if(HAL_UART_STATE_READY == huart->RxState && HAL_LOCKED == huart->Lock && i % 500 == 0)
            { 
                __HAL_UNLOCK(huart);
							  g_rx_error++;
                continue;
            }
        }
        else if(HAL_ERROR == status)
        { 
					  g_rx_other_error++;
            return status;
        }
        else if(HAL_TIMEOUT == status)
        {  
        }
    }
    if(HAL_OK != status)
    {
    }
    return status;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (++g_uart_rx_buf.tail >= RING_BUFFER_SIZE) {
        g_uart_rx_buf.tail = 0;
    }
		
		K_UART_Receive_IT(&huart1, (uint8_t *)&g_uart_rx_buf.data[g_uart_rx_buf.tail], 1);
}

void StartDefaultTask(void const * argument)
{
    /* USER CODE BEGIN 5 */
    /* Infinite loop */
    uint8_t temp;

    HAL_UART_Receive_IT(&huart1, &temp, 1);

    int iter = 0;
    int ret = AT_FAILED;
    while(iter++ <10) {
        ret = n720_normal_check("AT\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }
    osDelay(5);

    iter = 0;
    while(iter++ <10) {
        ret = n720_normal_check("ATE0\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }
    osDelay(5);


    iter = 0;
    while(iter++ <10) {
        ret = n720_normal_check("AT+CGSN\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }
    osDelay(5);
    iter = 0;
    while(iter++ <10) {
        //AT+CPIN?
        ret = n720_check_sim_card();
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
		printf("sim card check ret is %d\n", ret);
    if(ret == AT_FAILED) {
        return ;
    }

    osDelay(5);
    iter = 0;
    while(iter++ <10) {
        ret = n720_normal_check("AT+CIMI\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }

    osDelay(5);
    iter = 0;
    while(iter++ <10) {
        ret = n720_normal_check("AT+CREG=2\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }
    osDelay(5);
    iter = 0;
    while(iter++ <10) {
        ret = n720_check_creg();
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
		printf("creg check ret is %d\n", ret);
    if(ret == AT_FAILED) {
        return ;
    }

    osDelay(5);
    iter = 0;
    while(iter++ <10) {
        ret = n720_normal_check("AT+CREG=0\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }
    osDelay(5);
    iter = 0;
    while(iter++ <10) {
        ret = n720_normal_check("AT+CGATT=1\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }

    osDelay(5);
    iter = 0;
    while(iter++ <10) {
        ret = n720_check_cgatt();
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
		printf("cgatt check ret is %d\n", ret);
    if(ret == AT_FAILED) {
        return;
    }
    osDelay(5);
    iter = 0;
    while(iter++ <10) {
        ret = n720_normal_check("AT+CGDCONT=1,\"IP\", \"CMNET\"\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }
    osDelay(5);

    iter = 0;
		printf("try to get IP\n");
    while(iter++ <10) {
        ret = n720_normal_check("AT+XIIC=1\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }
    osDelay(5);
    iter = 0;
    while(iter++ <100) {
        ret = n720_check_xiic();
        if(ret == AT_SUCCESS) {
						printf("succeed to get IP\n");
            break;
        }
        osDelay(1);
    }

    if(ret == AT_FAILED) {
			  printf("failed to get IP, exit");
        return ;
    }
		printf("ready to run demo\n");
		
		
		osDelay(5);
    iter = 0;
    while(iter++ <10) {
        ret = n720_normal_check("AT$MYGPSPWR=1\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }
		
				osDelay(5);
    iter = 0;
    while(iter++ <10) {
        ret = n720_normal_check("AT$MYGNSSSEL=0\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }

						osDelay(5);
    iter = 0;
    while(iter++ <10) {
        ret = n720_normal_check("AT$MYGNSSMSG\r\n");
        if(ret == AT_SUCCESS) {
            break;
        }
        osDelay(1);
    }
    if(ret == AT_FAILED) {
        return ;
    }
		while(1)
		{
			
		}
		
    for(;;)
    {
        osDelay(10);
    }
    /* USER CODE END 5 */
}



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

#ifdef  USE_FULL_ASSERT
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
       tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
