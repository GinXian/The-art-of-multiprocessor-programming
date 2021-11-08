1. 关于`False Sharing`的形象化描述是有问题的，原因在于对`False Sharing`这个概念理解不透彻。
2. `lock_guard`只能使用`enable_if`来利用`SFINAE`拦截非`Lock`类型，而不能使用偏特化。
3. `CLHLock`和`MCSLock`的析构函数怎么处理`thread_local`结点？如果还是存指针的话，就要保证每个线程都执行一次析构函数，那么就意味着要保证锁的声明周期比线程短。而且，需要明确的知识点是**如何确定析构函数由哪个进程执行**。
4. 使用`unique_ptr`可以处理`CLHLock`的析构问题，而`MCSLock`根本不需要使用指针进行存储，因此析构时无需进行任何操作。为什么要使用`unique_ptr`呢？是因为一个`Node`有且仅有一个线程占用。
