#include <kernel.h>

/* ── Registradores necessários ── */

/* (1) Clock da Porta B — bit 10 do SIM_SCGC5 */
#define SIM_SCGC5    (*((volatile unsigned int*)0x40048038))

/* (2) Pin Control Register do pino 19 da Porta B */
#define PORTB_PCR19  (*((volatile unsigned int*)0x4004A04C))

/* (3) Direção dos pinos — Data Direction Register */
#define GPIOB_PDDR   (*((volatile unsigned int*)0x400FF054))

/* (4)/(6) Ligar e desligar o pino */
#define GPIOB_PSOR   (*((volatile unsigned int*)0x400FF044))  /* Set   → desliga LED */
#define GPIOB_PCOR   (*((volatile unsigned int*)0x400FF048))  /* Clear → liga LED   */

/* ── Função de espera ── */
void delayMs(int n) {
    int i;
    volatile int j;
    for (i = 0; i < n; i++)
        for (j = 0; j < 3500; j++) {}
}

int main(void)
{
    /* (1) Habilitar clock da Porta B */
    SIM_SCGC5 |= (1 << 10);

    /* (2) Configurar pino 19 como GPIO (MUX = 001) */
    PORTB_PCR19 = (1 << 8);

    /* (3) Setar direção do pino 19 como saída */
    GPIOB_PDDR |= (1 << 19);

    for (;;)
    {
        /* (4) Habilitar saída — liga LED verde (ativo baixo) */
        GPIOB_PCOR = (1 << 19);

        /* (5) Esperar 1 segundo */
        delayMs(1000);

        /* (6) Desabilitar saída — desliga LED */
        GPIOB_PSOR = (1 << 19);

        /* (7) Esperar 1 segundo */
        delayMs(1000);
    }

    return 0;
}