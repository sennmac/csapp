#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { QUEUE_CAPACITY = 8 };

/*
 * 有界环形队列：容量 QUEUE_CAPACITY
 * 一把 mutex 保护所有共享字段，两个条件变量协调生产者/消费者
 */
struct Queue {
  int data[QUEUE_CAPACITY];  /* 环形缓冲区 */
  int head;                  /* 消费者读取位置 */
  int tail;                  /* 生产者写入位置 */
  int count;                 /* 当前元素数量 */
  pthread_mutex_t lock;      /* 保护 head/tail/count/data */
  pthread_cond_t not_empty;  /* 通知消费者：队列有数据了 */
  pthread_cond_t not_full;   /* 通知生产者：队列有空位了 */
};

struct ProducerArgs {
  struct Queue *queue;
  int producer_id;
  int item_count;
};

struct ConsumerArgs {
  struct Queue *queue;
  int consumer_id;
  long long consumed_sum;
};

static void queue_init(struct Queue *queue) {
  memset(queue, 0, sizeof(*queue));
  pthread_mutex_init(&queue->lock, NULL);
  pthread_cond_init(&queue->not_empty, NULL);
  pthread_cond_init(&queue->not_full, NULL);
}

static void queue_destroy(struct Queue *queue) {
  pthread_mutex_destroy(&queue->lock);
  pthread_cond_destroy(&queue->not_empty);
  pthread_cond_destroy(&queue->not_full);
}

static void queue_push(struct Queue *queue, int value) {
  pthread_mutex_lock(&queue->lock);

  /*
   * 用 while 而非 if：cond_wait 可能虚假唤醒（spurious wakeup），
   * 醒来后必须重新检查条件，否则可能往满队列里写
   */
  while (queue->count == QUEUE_CAPACITY) {
    /* 原子地：释放 lock → 睡眠 → 被唤醒后重新持有 lock */
    pthread_cond_wait(&queue->not_full, &queue->lock);
  }

  /* 临界区：写数据并推进 tail 指针 */
  queue->data[queue->tail] = value;
  queue->tail = (queue->tail + 1) % QUEUE_CAPACITY;
  queue->count += 1;

  /* 通知一个正在等 not_empty 的消费者：队列有数据了 */
  pthread_cond_signal(&queue->not_empty);
  pthread_mutex_unlock(&queue->lock);
}

static int queue_pop(struct Queue *queue) {
  int value;

  pthread_mutex_lock(&queue->lock);

  /* 同理用 while：醒来后可能队列又被别的消费者取空了 */
  while (queue->count == 0) {
    pthread_cond_wait(&queue->not_empty, &queue->lock);
  }

  /* 临界区：读数据并推进 head 指针 */
  value = queue->data[queue->head];
  queue->head = (queue->head + 1) % QUEUE_CAPACITY;
  queue->count -= 1;

  /* 通知一个正在等 not_full 的生产者：队列有空位了 */
  pthread_cond_signal(&queue->not_full);
  pthread_mutex_unlock(&queue->lock);
  return value;
}

static void *producer_main(void *arg) {
  struct ProducerArgs *args = (struct ProducerArgs *)arg;
  int i;

  for (i = 0; i < args->item_count; ++i) {
    int value = args->producer_id * 1000 + i;
    queue_push(args->queue, value);
    printf("producer %d -> %d\n", args->producer_id, value);
  }

  return NULL;
}

static void *consumer_main(void *arg) {
  struct ConsumerArgs *args = (struct ConsumerArgs *)arg;

  while (true) {
    int value = queue_pop(args->queue);
    if (value < 0) {
      break;
    }
    args->consumed_sum += value;
    printf("consumer %d <- %d\n", args->consumer_id, value);
  }

  return NULL;
}

int main(int argc, char **argv) {
  struct Queue queue;
  pthread_t *producer_threads;
  pthread_t *consumer_threads;
  struct ProducerArgs *producer_args;
  struct ConsumerArgs *consumer_args;
  int producer_count;
  int consumer_count;
  int items_per_producer;
  int i;
  long long total_sum = 0;

  if (argc != 4) {
    fprintf(stderr, "usage: %s <producers> <consumers> <items_per_producer>\n", argv[0]);
    return 1;
  }

  producer_count = atoi(argv[1]);
  consumer_count = atoi(argv[2]);
  items_per_producer = atoi(argv[3]);
  if (producer_count <= 0 || consumer_count <= 0 || items_per_producer <= 0) {
    fprintf(stderr, "all counts must be > 0\n");
    return 1;
  }

  producer_threads = calloc((size_t)producer_count, sizeof(*producer_threads));
  consumer_threads = calloc((size_t)consumer_count, sizeof(*consumer_threads));
  producer_args = calloc((size_t)producer_count, sizeof(*producer_args));
  consumer_args = calloc((size_t)consumer_count, sizeof(*consumer_args));
  if (producer_threads == NULL || consumer_threads == NULL ||
      producer_args == NULL || consumer_args == NULL) {
    perror("calloc");
    free(producer_threads);
    free(consumer_threads);
    free(producer_args);
    free(consumer_args);
    return 1;
  }

  queue_init(&queue);

  for (i = 0; i < consumer_count; ++i) {
    consumer_args[i].queue = &queue;
    consumer_args[i].consumer_id = i;
    pthread_create(&consumer_threads[i], NULL, consumer_main, &consumer_args[i]);
  }

  for (i = 0; i < producer_count; ++i) {
    producer_args[i].queue = &queue;
    producer_args[i].producer_id = i;
    producer_args[i].item_count = items_per_producer;
    pthread_create(&producer_threads[i], NULL, producer_main, &producer_args[i]);
  }

  for (i = 0; i < producer_count; ++i) {
    pthread_join(producer_threads[i], NULL);
  }

  /*
   * 毒丸（poison pill）：每个消费者一颗
   * 消费者收到 value < 0 就退出循环，保证干净关闭
   */
  for (i = 0; i < consumer_count; ++i) {
    queue_push(&queue, -1);
  }

  for (i = 0; i < consumer_count; ++i) {
    pthread_join(consumer_threads[i], NULL);
    total_sum += consumer_args[i].consumed_sum;
  }

  printf("total consumed sum: %lld\n", total_sum);

  queue_destroy(&queue);
  free(producer_threads);
  free(consumer_threads);
  free(producer_args);
  free(consumer_args);
  return 0;
}
