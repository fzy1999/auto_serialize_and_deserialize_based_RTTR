# c2redis

### 介绍
    自动序列化带有指针嵌套的类存取redis
    使用clang ast进行代码自动注册, 但是需要再注册的类钱插入reflect(WithNonPulic), 类中插入rttr_friend_register
    不支持模板类自动注册
    支持optional基本类型, 暂不支持自定义optional类型
***
### 使用注意
1. 确保from_redis的生命周期不在传入的类之前结束, 否则类中的指针会被提前释放
2. 传入的类需要满足以下:
    - 没有const成员变量
    - 带有一个默认构造函数

### todo
1. 自动注册, 不需要插入代码
2. 支持模板自动注册
3. optional string array
4. bug in batch from: 不支持多层数组(map)中包含带指针的object (done)
5. muti thread  (done)
6. associaltive 和sequential 不能提前释放var, 会有些析构函数没法处理; 原始数据结构先释放, 再进行赋值
7. 直接使用普通指针保存数据, 指针直接赋值指针, 值使用拷贝? 
8. 从反射库底层进行处理, 自己维护类的map