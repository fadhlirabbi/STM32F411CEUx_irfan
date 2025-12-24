#include "stm32f4xx_hal.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdio.h>
#include <stdlib.h>

/* --- Konfigurasi Pin --- */
uint16_t led_pins[] = { GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5 };
uint16_t button_pins[] = { GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12 };

#define BUZZER_PIN GPIO_PIN_0
#define BUZZER_PORT GPIOB

/* Peripheral Handles */
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;

/* Variabel Game */
int sequence[20];
int current_step = 1;
uint32_t game_speed = 300;

/* Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
void PlayTone(uint16_t freq_delay, uint16_t duration);
void ShowSequence(void);
int WaitButtonPress(void);

void SysTick_Handler(void) { HAL_IncTick(); }

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();

    HAL_Delay(500);
    ssd1306_Init();

    while (1) {
        // --- LAYAR AWAL (Posisi Tengah) ---
        ssd1306_Fill(Black);
        ssd1306_SetCursor(2, 15); 
        ssd1306_WriteString("MEMORY GAME", Font_11x18, White);
        ssd1306_SetCursor(25, 40);
        ssd1306_WriteString("Press Any BTN", Font_7x10, White);
        ssd1306_UpdateScreen();
        
        WaitButtonPress(); // Tunggu user menekan tombol untuk mulai
        srand(HAL_GetTick()); 
        current_step = 1;
        HAL_Delay(500);

        while (current_step <= 20) {
            // 1. Ambil kecepatan dari Potensio (30ms - 500ms)
            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, 10);
            uint32_t pot = HAL_ADC_GetValue(&hadc1);
            HAL_ADC_Stop(&hadc1);
            game_speed = 30 + (pot * 470 / 4095); 

            // 2. Tampilkan Info Step (Hanya update saat ganti step)
            ssd1306_Fill(Black);
            char buf[32];
            sprintf(buf, "STEP: %d/20", current_step);
            ssd1306_SetCursor(15, 20);
            ssd1306_WriteString(buf, Font_11x18, White);
            ssd1306_UpdateScreen();
            HAL_Delay(800);

            // 3. Tambahkan urutan baru
            sequence[current_step - 1] = rand() % 5;

            // 4. Tunjukkan urutan LED (Fase Preview)
            ShowSequence();

            // 5. Input User
            int failed = 0;
            for (int i = 0; i < current_step; i++) {
                // Update layar status input
                ssd1306_SetCursor(30, 45);
                sprintf(buf, "Input: %d/%d", i, current_step);
                ssd1306_WriteString(buf, Font_7x10, White);
                ssd1306_UpdateScreen(); 

                int user_input = WaitButtonPress();
                
                if (user_input == sequence[i]) {
                    // Benar: Bunyi & Nyalakan LED sebentar
                    PlayTone(400, 100);
                    HAL_GPIO_WritePin(GPIOB, led_pins[user_input], GPIO_PIN_SET);
                    HAL_Delay(100); 
                    HAL_GPIO_WritePin(GPIOB, led_pins[user_input], GPIO_PIN_RESET);
                    
                    // Update tampilan setelah input berhasil
                    ssd1306_SetCursor(30, 45);
                    sprintf(buf, "Input: %d/%d", i + 1, current_step);
                    ssd1306_WriteString(buf, Font_7x10, White);
                    ssd1306_UpdateScreen();
                } else {
                    failed = 1;
                    break;
                }
            }

            if (failed) {
                // Game Over
                PlayTone(1200, 800);
                ssd1306_Fill(Black);
                ssd1306_SetCursor(25, 20);
                ssd1306_WriteString("WRONG!", Font_11x18, White);
                ssd1306_UpdateScreen();
                HAL_Delay(2000);
                break; 
            } else {
                current_step++;
                HAL_Delay(400);
            }
        }

        if (current_step > 20) {
            // Menang
            ssd1306_Fill(Black);
            ssd1306_SetCursor(15, 20);
            ssd1306_WriteString("VICTORY!", Font_11x18, White);
            ssd1306_UpdateScreen();
            HAL_Delay(5000);
        }
    }
}

/* --- LOGIKA PERMAINAN --- */

void ShowSequence(void) {
    for (int i = 0; i < current_step; i++) {
        HAL_GPIO_WritePin(GPIOB, led_pins[sequence[i]], GPIO_PIN_SET);
        
        // Buzzer menyatu dengan durasi game_speed agar timing sinkron
        uint32_t start = HAL_GetTick();
        while(HAL_GetTick() - start < game_speed) {
            HAL_GPIO_TogglePin(BUZZER_PORT, BUZZER_PIN);
            for (volatile int j = 0; j < 500; j++); 
        }
        
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, led_pins[sequence[i]], GPIO_PIN_RESET);
        HAL_Delay(game_speed); // Jeda antar kedipan
    }
}

int WaitButtonPress(void) {
    while (1) {
        for (int i = 0; i < 5; i++) {
            if (HAL_GPIO_ReadPin(GPIOA, button_pins[i]) == GPIO_PIN_RESET) {
                while(HAL_GPIO_ReadPin(GPIOA, button_pins[i]) == GPIO_PIN_RESET);
                HAL_Delay(30); // Debounce cepat
                return i;
            }
        }
    }
}

void PlayTone(uint16_t freq_delay, uint16_t duration) {
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < duration) {
        HAL_GPIO_TogglePin(BUZZER_PORT, BUZZER_PIN);
        for (volatile int i = 0; i < freq_delay; i++);
    }
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}

/* --- INISIALISASI PERIPHERAL --- */

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_ADC1_Init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    __HAL_RCC_ADC1_CLK_ENABLE();
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

static void MX_I2C1_Init(void) {
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    HAL_I2C_Init(&hi2c1);
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOB, 0x3F, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}