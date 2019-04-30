/* function:创建线程池
 * value:
 * 	max_thread_num:线程池中最大线程数.
 * */
void pool_init (int max_thread_num);
/* function:销毁线程池
 * value:
 * return value:
 * 	0为成功,小于0为失败.
 * */
int pool_destroy ();
/* function:向线程池的任务队列中添加一个任务
 * value:
 * 	process:线程执行任务
 * 	arg:线程参数
 * return value:
 * 	0为成功,小于0为失败.
 * */
int pool_add_worker (void *(*process) (void *arg), void *arg);


