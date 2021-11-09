1. ~~关于`False Sharing`的形象化描述是有问题的，原因在于对`False Sharing`这个概念理解不透彻。~~
2. `lock_guard`只能使用`enable_if`来利用`SFINAE`拦截非`Lock`类型，而不能使用偏特化。
3. `CLHLock`和`MCSLock`的析构函数怎么处理`thread_local`结点？如果还是存指针的话，就要保证每个线程都执行一次析构函数，那么就意味着要保证锁的声明周期比线程短。而且，需要明确的知识点是**如何确定析构函数由哪个进程执行**。
4. 使用~~`unique_ptr`~~`两个shared_ptr`可以处理`CLHLock`的析构问题，而`MCSLock`根本不需要使用指针进行存储，因此析构时无需进行任何操作。为什么不能使用`unique_ptr`呢？因为在`myNode`重新指向新节点的时候，`unique_ptr`原来指向的结点会引用清零，导致其被析构，这样就会导致一直需要重新分配空间。而使用两个`shared_ptr`就可以复用旧的结点，并且同时实现在线程生命周期结束的时候自动释放结点空间的效果。
5. `memory_order`的使用暂时没有掌握。
6. `std::atomic_compare_exchange`在失败时会修改`expected`，注意是否需要重新赋值。
7. 使用`volatile`关键字避免编译器将类似`while(flag) {}`这样的代码优化掉。
8. 添加了`multicore`作为`namespace`，避免命名冲突。
