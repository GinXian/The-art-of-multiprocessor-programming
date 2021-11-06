1. 关于`False Sharing`的形象化描述是有问题的，原因在于对`False Sharing`这个概念理解不透彻。
2. `lock_guard`只能使用`enable_if`来利用`SFINAE`拦截非`Lock`类型，而不能使用偏特化。
3. `CLHLock`和`MCSLock`的析构函数怎么处理`thread_local`结点？如果还是存指针的话，就要保证每个线程都执行一次析构函数，那么就意味着要保证锁的声明周期比线程短。而且，需要明确的知识点是**如何确定析构函数由哪个进程执行**。

