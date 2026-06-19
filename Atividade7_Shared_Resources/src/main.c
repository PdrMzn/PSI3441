/*************************** SEM SINCRONIZAÇÃO ***************************/

// #include <zephyr/kernel.h>

// #define STACK_SIZE 1024

// /* Prioridades iguais para as duas threads */
// #define PRIO_PADEIRO  5
// #define PRIO_CLIENTE  5

// /* Variável compartilhada: quantidade de pães na vitrine */
// volatile int saldo_vitrine = 0;

// K_THREAD_STACK_DEFINE(stack_padeiro, STACK_SIZE);
// K_THREAD_STACK_DEFINE(stack_cliente, STACK_SIZE);
// struct k_thread thread_padeiro_data;
// struct k_thread thread_cliente_data;

// /* ===== Thread Padeiro: produz 1 pão a cada 1 segundo ===== */
// void thread_padeiro(void *a, void *b, void *c)
// {
//     while (1) {
//         saldo_vitrine++;  /* produção */
//         printk("[PADEIRO] Produziu pao. Vitrine: %d\n", saldo_vitrine);
//         k_msleep(1000);
//     }
// }

// /* ===== Thread Cliente: retira 1 pão a cada 1.5 segundos ===== */
// void thread_cliente(void *a, void *b, void *c)
// {
//     while (1) {
//         saldo_vitrine--;  /* consumo */
//         printk("[CLIENTE] Retirou pao. Vitrine: %d\n", saldo_vitrine);
//         k_msleep(1500);
//     }
// }

// void main(void)
// {
//     printk("=== Padaria - Parte 1: SEM sincronizacao ===\n\n");

//     k_thread_create(&thread_padeiro_data, stack_padeiro, STACK_SIZE,
//                     thread_padeiro, NULL, NULL, NULL,
//                     PRIO_PADEIRO, 0, K_NO_WAIT);

//     k_thread_create(&thread_cliente_data, stack_cliente, STACK_SIZE,
//                     thread_cliente, NULL, NULL, NULL,
//                     PRIO_CLIENTE, 0, K_NO_WAIT);

//     while (1) {
//         k_sleep(K_FOREVER);
//     }
// }

/*************************** MUTEX ***************************/

// #include <zephyr/kernel.h>

// #define STACK_SIZE 1024
// #define PRIO_PADEIRO  5
// #define PRIO_CLIENTE  5

// volatile int saldo_vitrine = 0;

// /* Mutex para proteger o acesso a saldo_vitrine */
// K_MUTEX_DEFINE(mutex_vitrine);

// K_THREAD_STACK_DEFINE(stack_padeiro, STACK_SIZE);
// K_THREAD_STACK_DEFINE(stack_cliente, STACK_SIZE);
// struct k_thread thread_padeiro_data;
// struct k_thread thread_cliente_data;

// void thread_padeiro(void *a, void *b, void *c)
// {
//     while (1) {
//         /* Trava o mutex antes de acessar a variavel compartilhada */
//         k_mutex_lock(&mutex_vitrine, K_FOREVER);
//         saldo_vitrine++;
//         printk("[PADEIRO] Produziu pao. Vitrine: %d\n", saldo_vitrine);
//         k_mutex_unlock(&mutex_vitrine);

//         k_msleep(1000);
//     }
// }

// void thread_cliente(void *a, void *b, void *c)
// {
//     while (1) {
//         k_mutex_lock(&mutex_vitrine, K_FOREVER);
//         saldo_vitrine--;
//         printk("[CLIENTE] Retirou pao. Vitrine: %d\n", saldo_vitrine);
//         k_mutex_unlock(&mutex_vitrine);

//         k_msleep(1500);
//     }
// }

// void main(void)
// {
//     printk("=== Padaria - Parte 2: com MUTEX ===\n\n");

//     k_thread_create(&thread_padeiro_data, stack_padeiro, STACK_SIZE,
//                     thread_padeiro, NULL, NULL, NULL,
//                     PRIO_PADEIRO, 0, K_NO_WAIT);

//     k_thread_create(&thread_cliente_data, stack_cliente, STACK_SIZE,
//                     thread_cliente, NULL, NULL, NULL,
//                     PRIO_CLIENTE, 0, K_NO_WAIT);

//     while (1) {
//         k_sleep(K_FOREVER);
//     }
// }

/*************************** SEMÁFORO ***************************/

#include <zephyr/kernel.h>

#define STACK_SIZE 1024
#define PRIO_PADEIRO  5
#define PRIO_CLIENTE  5

#define CAPACIDADE_MAX 10

volatile int saldo_vitrine = 0;

/* Semaforo que conta os PAES disponiveis (inicia em 0, max 10) */
K_SEM_DEFINE(sem_paes, 0, CAPACIDADE_MAX);
/* Semaforo que conta os ESPACOS livres (inicia em 10, max 10) */
K_SEM_DEFINE(sem_espacos, CAPACIDADE_MAX, CAPACIDADE_MAX);
/* Mutex para proteger o acesso a saldo_vitrine */
K_MUTEX_DEFINE(mutex_vitrine);

K_THREAD_STACK_DEFINE(stack_padeiro, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_cliente, STACK_SIZE);
struct k_thread thread_padeiro_data;
struct k_thread thread_cliente_data;

void thread_padeiro(void *a, void *b, void *c)
{
    while (1) {
        /* Espera ter ESPACO livre na vitrine (decrementa sem_espacos) */
        k_sem_take(&sem_espacos, K_FOREVER);

        /* Regiao critica: produz o pao */
        k_mutex_lock(&mutex_vitrine, K_FOREVER);
        saldo_vitrine++;
        printk("[PADEIRO] Produziu pao. Vitrine: %d\n", saldo_vitrine);
        k_mutex_unlock(&mutex_vitrine);

        /* Sinaliza que ha mais um PAO disponivel (incrementa sem_paes) */
        k_sem_give(&sem_paes);

        k_msleep(1000);
    }
}

void thread_cliente(void *a, void *b, void *c)
{
    while (1) {
        /* Espera ter PAO disponivel (decrementa sem_paes) */
        k_sem_take(&sem_paes, K_FOREVER);

        /* Regiao critica: retira o pao */
        k_mutex_lock(&mutex_vitrine, K_FOREVER);
        saldo_vitrine--;
        printk("[CLIENTE] Retirou pao. Vitrine: %d\n", saldo_vitrine);
        k_mutex_unlock(&mutex_vitrine);

        /* Sinaliza que ha mais um ESPACO livre (incrementa sem_espacos) */
        k_sem_give(&sem_espacos);

        k_msleep(1500);
    }
}

void main(void)
{
    printk("=== Padaria - Parte 3: com SEMAFOROS ===\n");
    printk("Capacidade maxima da vitrine: %d paes\n\n", CAPACIDADE_MAX);

    k_thread_create(&thread_padeiro_data, stack_padeiro, STACK_SIZE,
                    thread_padeiro, NULL, NULL, NULL,
                    PRIO_PADEIRO, 0, K_NO_WAIT);

    k_thread_create(&thread_cliente_data, stack_cliente, STACK_SIZE,
                    thread_cliente, NULL, NULL, NULL,
                    PRIO_CLIENTE, 0, K_NO_WAIT);

    while (1) {
        k_sleep(K_FOREVER);
    }
}