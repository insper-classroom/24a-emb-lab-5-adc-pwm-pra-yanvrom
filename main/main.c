/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

QueueHandle_t xQueueAdc;

int PIN_X = 26;
int PIN_Y = 27;

typedef struct adc {
    int axis;
    int val;
} adc_t;

void x_task(){
    while(true){
        adc_select_input(0);
        uint16_t result = adc_read();
        int x = (result - 2047)/8.03;
        if(x < 165 && x > -165){
            x = 0;
        }
        adc_t data = {
            .axis = 0,
            .val = x
        };
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(){
    while(true){
        adc_select_input(1);
        uint16_t result = adc_read();
        int y = (result - 2047)/8.03;
        if(y < 165 && y > -165){
            y = 0;
        }
        adc_t data = {
            .axis = 1,
            .val = y
        };
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void uart_task(void *p) {
    adc_t data;

    while (1) {
        xQueueReceive(xQueueAdc, &data, portMAX_DELAY);
        //jogar na uart

        // AXIS VAL_1 VAL_0 EOP
        // Onde cada um dos termos do datagrama é um composto por 8 bits que indica:

        // AXIS: 0 ou 1 para indicar eixo X ou eixo Y.
        // VAL_1: Byte mais significativo (MSB)do valor do movimento do eixo
        // VAL_0: Byte menos significativo (LSB) do valor do movimento do
        // EOP: -1 indica fim de pacote

        int valor = data.val;
        int msb = valor >> 8;
        int lsb = valor & 0xFF;

        //pegar os 8 bits menos significativos de axis + espaço + 8 bits de val_1 + espaço + 8 bits de val_0 + espaço + 11111111, colocar em binário
        //enviar para a uart
        //passar para binario com 8 bits axis, val_1 e val_0

        uart_putc_raw(uart0, data.axis);
        uart_putc_raw(uart0, lsb);
        uart_putc_raw(uart0, msb);
        uart_putc_raw(uart0, -1);


    }
}

int main() {
    stdio_init_all();
    adc_init();

    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(27);
    adc_gpio_init(26);

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    xTaskCreate(x_task, "x_task", 4096, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
