#include <kernel.h>

/* ── Clock ── */
#define SIM_SCGC5   (*((volatile unsigned int*)0x40048038))  /* Clock portas GPIO */
#define SIM_SCGC6   (*((volatile unsigned int*)0x4004803C))  /* Clock ADC0 */

/* ── Porta B — LED verde (PTB19) e LED azul (PTD1) ── */
#define PORTB_PCR19 (*((volatile unsigned int*)0x4004A04C))  /* LED verde */
#define PORTD_PCR1  (*((volatile unsigned int*)0x4004C004))  /* LED azul  */

/* ── Porta E — canal ADC (PTE20 = ADC0_SE0) ── */
#define PORTE_PCR20 (*((volatile unsigned int*)0x4004D050))

/* ── GPIO ── */
#define GPIOB_PDDR  (*((volatile unsigned int*)0x400FF054))
#define GPIOB_PSOR  (*((volatile unsigned int*)0x400FF044))  /* desliga */
#define GPIOB_PCOR  (*((volatile unsigned int*)0x400FF048))  /* liga    */

#define GPIOD_PDDR  (*((volatile unsigned int*)0x400FF0D4))
#define GPIOD_PSOR  (*((volatile unsigned int*)0x400FF0C4))  /* desliga */
#define GPIOD_PCOR  (*((volatile unsigned int*)0x400FF0C8))  /* liga    */

/* ── ADC0 ── */
#define ADC0_CFG1   (*((volatile unsigned int*)0x4003B008))  /* clock e resolução */
#define ADC0_SC2    (*((volatile unsigned int*)0x4003B020))  /* trigger */
#define ADC0_SC1A   (*((volatile unsigned int*)0x4003B000))  /* canal + inicia conversão */
#define ADC0_RA     (*((volatile unsigned int*)0x4003B010))  /* resultado */

/* Bit COCO (bit 7 do SC1A): 1 = conversão completa */
#define ADC_COCO    (1 << 7)

void delayMs(int n) {
    int i;
    volatile int j;
    for (i = 0; i < n; i++)
        for (j = 0; j < 3500; j++) {}
}

int main(void)
{
    /* (1) Habilitar clock da Porta B, D e E */
    SIM_SCGC5 |= (1 << 10) | (1 << 12) | (1 << 13); /* PORTB, PORTD, PORTE */

    /* (2) Configurar pinos dos LEDs como GPIO (MUX = 001) */
    PORTB_PCR19 = (1 << 8);   /* LED verde — PTB19 */
    PORTD_PCR1  = (1 << 8);   /* LED azul  — PTD1  */

    /* Configurar pino PTE20 como analógico (MUX = 000) */
    PORTE_PCR20 = 0;

    /* (3) Setar direção dos pinos LED como saída */
    GPIOB_PDDR |= (1 << 19);  /* PTB19 saída */
    GPIOD_PDDR |= (1 << 1);   /* PTD1  saída */

    /* LEDs começam apagados (ativo baixo) */
    GPIOB_PSOR = (1 << 19);
    GPIOD_PSOR = (1 << 1);

    /* (3) Habilitar clock do ADC0 — bit 27 do SIM_SCGC6 */
    SIM_SCGC6 |= (1 << 27);

    /* (4) Trigger por software */
    ADC0_SC2 = 0x00;

    /* (5) Bus clock, resolução 12 bits single-ended */
    ADC0_CFG1 = 0x04;  /* ADICLK=00 (bus clock), MODE=01 (12 bits) */

    for (;;)
    {
        ADC0_SC1A = 0x00;
        while (!(ADC0_SC1A & ADC_COCO)) {}
        unsigned int result = ADC0_RA;

        printk("ADC: %d\n", result);  // mostra o valor lido

        if (result > 2048)
        {
            GPIOD_PCOR = (1 << 1);
            GPIOB_PSOR = (1 << 19);
        }
        else
        {
            GPIOB_PCOR = (1 << 19);
            GPIOD_PSOR = (1 << 1);
        }

        delayMs(500);
    }

    return 0;
}