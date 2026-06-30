#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>

/* ===== Configuração ADC ===== */
#define ADC_RESOLUTION       12
#define ADC_GAIN             ADC_GAIN_1
#define ADC_REFERENCE        ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT
#define ADC_CHANNEL_ID       0

static int16_t sample_buffer;
static const struct device *const adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));

/* ===== Taxa de aquisição — MUDE AQUI A CADA RODADA ===== */
/* Rodadas: 1000, 500, 200, 100, 50 */
#define PERIODO_AMOSTRA_US   500

/* ===== Filtro FIR ===== */
#define USAR_FILTRO   0
#define FIR_TAPS      8

static int16_t fir_buffer[FIR_TAPS] = {0};
static int fir_index = 0;

int16_t aplicar_fir(int16_t nova_amostra)
{
    fir_buffer[fir_index] = nova_amostra;
    fir_index = (fir_index + 1) % FIR_TAPS;
    int32_t soma = 0;
    for (int i = 0; i < FIR_TAPS; i++) soma += fir_buffer[i];
    return (int16_t)(soma / FIR_TAPS);
}

/* ===== FIFO ===== */
struct amostra {
    void *fifo_reserved;
    int64_t timestamp_us;
    int16_t valor;
};
K_FIFO_DEFINE(fifo_amostras);

/* ===== Contadores ===== */
static volatile uint32_t amostras_adquiridas = 0;  /* produzidas com sucesso */
static volatile uint32_t amostras_perdidas   = 0;  /* malloc falhou */
static volatile uint32_t amostras_consumidas = 0;  /* lidas da FIFO */

/* ===== Threads ===== */
#define STACK_SIZE 1024
#define PRIO_AQUISICAO   4
#define PRIO_COMUNICACAO 5
#define PRIO_STATS       6

K_THREAD_STACK_DEFINE(stack_aq, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_com, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_stats, STACK_SIZE);
struct k_thread thread_aq_data;
struct k_thread thread_com_data;
struct k_thread thread_stats_data;

/* Thread de aquisição (produtor) */
void thread_aquisicao(void *a, void *b, void *c)
{
    struct adc_sequence sequence = {
        .channels    = BIT(ADC_CHANNEL_ID),
        .buffer      = &sample_buffer,
        .buffer_size = sizeof(sample_buffer),
        .resolution  = ADC_RESOLUTION,
    };

    while (1) {
        if (adc_read(adc_dev, &sequence) == 0) {
            struct amostra *amo = k_malloc(sizeof(struct amostra));
            if (amo != NULL) {
                amo->timestamp_us = k_uptime_ticks() * 1000000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
#if USAR_FILTRO
                amo->valor = aplicar_fir(sample_buffer);
#else
                amo->valor = sample_buffer;
#endif
                k_fifo_put(&fifo_amostras, amo);
                amostras_adquiridas++;
            } else {
                amostras_perdidas++;
            }
        }
        k_usleep(PERIODO_AMOSTRA_US);
    }
}

/* Thread de comunicação (consumidor) — apenas consome, NÃO imprime cada amostra */
void thread_comunicacao(void *a, void *b, void *c)
{
    while (1) {
        struct amostra *amo = k_fifo_get(&fifo_amostras, K_FOREVER);
        if (amo != NULL) {
            amostras_consumidas++;
            k_free(amo);
        }
    }
}

/* Thread de estatísticas — imprime a cada 2 segundos */
void thread_stats(void *a, void *b, void *c)
{
    uint32_t ant_adq = 0;

    while (1) {
        k_msleep(2000);

        uint32_t adq = amostras_adquiridas;
        uint32_t perd = amostras_perdidas;
        uint32_t cons = amostras_consumidas;

        /* Taxa efetiva = amostras adquiridas nos ultimos 2s / 2 */
        uint32_t taxa = (adq - ant_adq) / 2;
        ant_adq = adq;

        uint32_t total = adq + perd;
        uint32_t perc = (total > 0) ? (perd * 100 / total) : 0;

        printk("STATS | periodo=%dus | taxa_efetiva=%u Hz | adq=%u cons=%u perd=%u (%u%% perda)\n",
               PERIODO_AMOSTRA_US, taxa, adq, cons, perd, perc);
    }
}

void main(void)
{
    printk("\n=== Experimento de Taxa - Periodo configurado: %d us ===\n", PERIODO_AMOSTRA_US);

    if (!device_is_ready(adc_dev)) {
        printk("ADC nao esta pronto\n");
        return;
    }

    struct adc_channel_cfg channel_cfg = {
        .gain             = ADC_GAIN,
        .reference        = ADC_REFERENCE,
        .acquisition_time = ADC_ACQUISITION_TIME,
        .channel_id       = ADC_CHANNEL_ID,
        .differential     = 0,
    };
    adc_channel_setup(adc_dev, &channel_cfg);

    k_thread_create(&thread_aq_data, stack_aq, STACK_SIZE,
                    thread_aquisicao, NULL, NULL, NULL,
                    PRIO_AQUISICAO, 0, K_NO_WAIT);
    k_thread_create(&thread_com_data, stack_com, STACK_SIZE,
                    thread_comunicacao, NULL, NULL, NULL,
                    PRIO_COMUNICACAO, 0, K_NO_WAIT);
    k_thread_create(&thread_stats_data, stack_stats, STACK_SIZE,
                    thread_stats, NULL, NULL, NULL,
                    PRIO_STATS, 0, K_NO_WAIT);

    while (1) {
        k_sleep(K_FOREVER);
    }
}