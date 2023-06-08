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
4. bug in batch from: 不支持多层数组(map)中包含带指针的object
5. muti thread