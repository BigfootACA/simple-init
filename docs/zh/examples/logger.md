# Logger日志示例

## 在新进程中，需要使用以下代码初始化日志服务

```C
open_socket_logfd_default();
```

## 普通日志记录

```C
#include"logger.h"
#define TAG "test"

void example_logger_1(void){
	open_socket_logfd_default();

	tlog_debug("This is a debug message");

	tlog_notice("My PID is %d",getpid());
}
```

```
[2021/12/22 17:54:11] <DEBUG>  test[100]: This is a debug message
[2021/12/22 17:54:11] <WARN>   test[100]: My PID is 100
```

## 捕获错误输出

```C
#include"logger.h"
#define TAG "test"

void example_logger_2(void){
	open_socket_logfd_default();

	close(-1);
	telog_warn("close say");
}
```

```
[2021/12/22 17:55:27] <WARN>   test[100]: close say: Bad file descriptor
```
