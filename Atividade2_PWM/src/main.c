#include <kernel.h>
#include <device.h>
#include <drivers/gpio.h>
#include <pwm_z42.h>

// Define o valor do registrador MOD do TPM para configurar o período do PWM
#define TPM_MODULE 1000         // Define a frequência do PWM fpwm = (TPM_CLK / (TPM_MODULE * PS))
// Valores de duty cycle correspondentes a diferentes larguras de pulso
uint16_t duty_50  = TPM_MODULE/2;       // 50% de duty cycle (meio brilho)

int main(void)
{
    // Inicializa o módulo TPM2 com:
    // - base do TPMx
    // - fonte de clock PLL/FLL (TPM_CLK)
    // - valor do registrador MOD
    // - tipo de clock (TPM_CLK)
    // - prescaler de 1 a 128 (PS)
    // - modo de operação EDGE_PWM
    pwm_tpm_Init(TPM2, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);

    // Inicializa o canal 0 do TPM2 para gerar sinal PWM na porta GPIOB_18
    // - modo TPM_PWM_H (nível alto durante o pulso)
    pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, 18);

    // Define o valor do duty cycle: nesse caso, duty_100 (LED quase desligado)
    pwm_tpm_CnV(TPM2, 0, duty_50);

    // Loop infinito
    for (;;)
    {
        uint16_t duty;
        for (duty = TPM_MODULE; duty >0; duty -= 10)
        {
            pwm_tpm_CnV(TPM2, 0, duty);
            k_msleep(20);
        }
    }
}


// #include <zephyr.h>
// #include <device.h>
// #include <drivers/gpio.h>
// #include <sys/printk.h>
// #include "pwm.h"

// #define TPM_MODULE 3749

// void main(void)
// {
//     printk("Iniciando teste LED RGB\n");

//     // 1. Testa Vermelho (TPM2 CH0)
//     printk("Testando vermelho...\n");
//     pwm_tpm_Init(TPM2_BASE_PTR, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);
//     pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, 18);
//     pwm_tpm_CnV(TPM2, 0, 3000);
//     k_msleep(2000);

//     // 2. Testa Verde (TPM0 CH1)
//     printk("Testando verde...\n");
//     pwm_tpm_Init(TPM0_BASE_PTR, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);
//     pwm_tpm_Ch_Init(TPM0, 1, TPM_PWM_H, GPIOB, 19);
//     pwm_tpm_CnV(TPM0, 1, 1500);
//     k_msleep(2000);

//     // 3. Combinação Laranja
//     printk("Tentando laranja...\n");
//     pwm_tpm_CnV(TPM2, 0, 3000);  // Vermelho
//     pwm_tpm_CnV(TPM0, 1, 750);   // Verde

//     while (1) {
//         k_msleep(1000);
//     }
// }