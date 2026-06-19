#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>

/* ============ Configuração ADC ============ */
#define ADC_RESOLUTION       12
#define ADC_GAIN             ADC_GAIN_1
#define ADC_REFERENCE        ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT
#define ADC_CHANNEL_ID       0
#define ADC_VREF_MV          3300

static int16_t sample_buffer;
static const struct device *const adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));

/* ============ Acelerômetro ============ */
static const struct device *const accel = DEVICE_DT_GET(DT_NODELABEL(mma8451q));

/* ============ LED e Botão ============ */
#define LED_NODE    DT_ALIAS(led1)
#define BUTTON_NODE DT_NODELABEL(user_button_0)
static const struct gpio_dt_spec led    = GPIO_DT_SPEC_GET(LED_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

/* ============ Variável de modo (compartilhada) ============ */
/* 0 = Modo ADC (só ADC) | 1 = Modo Completo (ADC + acelerômetro) */
volatile int modo_completo = 0;

/* ============ ISR do botão: alterna o modo ============ */
void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    modo_completo = !modo_completo;
    gpio_pin_set_dt(&led, modo_completo);  /* LED aceso = modo completo */
}

/* ============ Configuração das threads ============ */
#define STACK_SIZE 1024

/* Prioridades — comece com AMBAS IGUAIS (item 6 da atividade) */
#define PRIO_ADC    5
#define PRIO_ACCEL  3

K_THREAD_STACK_DEFINE(stack_adc, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_accel, STACK_SIZE);
struct k_thread thread_adc_data;
struct k_thread thread_accel_data;

/* ============ Thread do ADC — a cada 500ms ============ */
void thread_adc(void *a, void *b, void *c)
{
    struct adc_sequence sequence = {
        .channels    = BIT(ADC_CHANNEL_ID),
        .buffer      = &sample_buffer,
        .buffer_size = sizeof(sample_buffer),
        .resolution  = ADC_RESOLUTION,
    };

    while (1) {
        int err = adc_read(adc_dev, &sequence);
        if (err == 0) {
            int32_t mv = sample_buffer;
            adc_raw_to_millivolts(ADC_VREF_MV, ADC_GAIN, ADC_RESOLUTION, &mv);
            printk("[ADC] %d raw, %d mV\n", sample_buffer, mv);
        } else {
            printk("[ADC] Falha na leitura: %d\n", err);
        }
        k_msleep(500);
    }
}

/* ============ Thread do acelerômetro — a cada 1000ms ============ */
void thread_accel(void *a, void *b, void *c)
{
    struct sensor_value ax, ay, az;

    while (1) {
        /* Só imprime se estiver no modo completo */
        if (modo_completo) {
            if (sensor_sample_fetch(accel) == 0) {
                sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &ax);
                sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &ay);
                sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &az);
                printk("[ACEL] X=%d.%06d Y=%d.%06d Z=%d.%06d\n",
                       ax.val1, abs(ax.val2),
                       ay.val1, abs(ay.val2),
                       az.val1, abs(az.val2));
            } else {
                printk("[ACEL] Erro ao ler sensor\n");
            }
        }
        k_msleep(1000);
    }
}

void main(void)
{
    printk("=== Atividade 6 - Threads ===\n");

    /* Verifica dispositivos */
    if (!device_is_ready(adc_dev)) {
        printk("ADC nao esta pronto\n");
        return;
    }
    if (!device_is_ready(accel)) {
        printk("Acelerometro nao esta pronto\n");
        return;
    }
    if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&button)) {
        printk("LED ou botao nao estao prontos\n");
        return;
    }

    /* Configura canal ADC */
    struct adc_channel_cfg channel_cfg = {
        .gain             = ADC_GAIN,
        .reference        = ADC_REFERENCE,
        .acquisition_time = ADC_ACQUISITION_TIME,
        .channel_id       = ADC_CHANNEL_ID,
        .differential     = 0,
    };
    adc_channel_setup(adc_dev, &channel_cfg);

    /* Configura LED e botão */
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);

    /* Configura interrupção do botão na borda de descida */
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_FALLING);
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    printk("Modo inicial: ADC (LED apagado)\n");
    printk("Pressione o botao para alternar modos\n\n");

    /* Cria as duas threads */
    k_thread_create(&thread_adc_data, stack_adc, STACK_SIZE,
                    thread_adc, NULL, NULL, NULL,
                    PRIO_ADC, 0, K_NO_WAIT);

    k_thread_create(&thread_accel_data, stack_accel, STACK_SIZE,
                    thread_accel, NULL, NULL, NULL,
                    PRIO_ACCEL, 0, K_NO_WAIT);

    /* Main não precisa fazer mais nada */
    while (1) {
        k_sleep(K_FOREVER);
    }
}